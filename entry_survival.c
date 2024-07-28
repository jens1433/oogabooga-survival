
#define m4_identity m4_scalar(1.0)

#define v2_as_v3(v) v3((v).x, (v).y, 0)

#define v2_zero v2(0, 0)

#define v2_one v2(1, 1)

inline f32 v2_cross(Vector2 a, Vector2 b) {
	return a.x * b.y - a.y * b.x;
}

inline f32 v2_look_at_radians(Vector2 from, Vector2 to) {
	return atan2f(to.y - from.y, to.x - from.x);
}

Vector2 screen_to_world(Vector2 screen) {
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (screen.x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (screen.y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	
	return world_pos.xy;
}

Vector2 get_mouse_world_pos() {
	return screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y));
}

bool almost_equals(float a, float b, float epsilon) {
 	return fabs(a - b) <= epsilon;
}

bool animate_f32(f32* value, f32 target, f32 delta_t, f32 rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f)) {
		*value = target;
		return true;
	}
	return false;
}

void animate_v2(Vector2* value, Vector2 target, f32 delta_t, f32 rate) {
	animate_f32(&(value->x), target.x, delta_t, rate);
	animate_f32(&(value->y), target.y, delta_t, rate);
}

#define TIMERS \
	X(tool_cooldown, 0.2)

#define X(ID, Length) TIMER_ ## ID,
typedef enum TimerID {
	TIMER_nil,
	TIMERS
	TIMER_MAX
} TimerID;
#undef X

typedef struct Timer {
	f32 length;
	f32 progress;
	bool finished;
} Timer;

#define X(ID, Length) (Timer) { Length, 0, true},
Timer timers[] = {
	(Timer) { 0, 0, true},
	TIMERS
};
#undef X

void update_timers(f32 delta_t) {
	for (int i = 0; i < TIMER_MAX; i++) {
		Timer* timer = &timers[i];
		if (timer->finished)
			continue;
		
		timer->progress += delta_t;
		if (timer->progress >= timer->length)
			timer->finished = true;
	}
}

Timer* get_timer(TimerID timerID) {
	if (timerID < TIMER_MAX) {
		return &timers[timerID];
	}

	return &timers[0];
}

bool try_start_timer(TimerID timerID) {
	Timer* timer = get_timer(timerID);
	if (timer->finished) {
		timer->finished = false;
		timer->progress = 0;
		return true;
	}
	return false;
}

#define SPRITES \
	X(player, "res/sprites/character.png") \
	X(pickaxe, "res/sprites/pickaxe.png") \
	X(rock_small, "res/sprites/rock-small.png") \
	X(rock_medium, "res/sprites/rock-medium.png") \
	X(rock_large, "res/sprites/rock-large.png")

typedef struct Sprite {
	Gfx_Image* image;
} Sprite;

#define X(ID, Path) SPRITE_ ## ID,
typedef enum SpriteID {
	SPRITE_nil,
	SPRITES
	SPRITE_MAX,
} SpriteID;
#undef X

Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id) {
	if (id < SPRITE_MAX) {
		return &sprites[id];
	}

	return &sprites[0];
}

Vector2 get_sprite_size(Sprite* sprite) {
	return (Vector2) { sprite->image->width, sprite->image->height };
}

#define ENTITIES \
	X(player, SPRITE_player, "Player") \

#define X(Class, Sprite, Name) class_## Class,
typedef enum EntityClass {
	class_nil,
	ENTITIES
	CLASS_MAX
} EntityClass;
#undef X

typedef struct Entity {
	bool is_valid;
	EntityClass class;
	Vector2 pos;
	bool visible;
	SpriteID sprite_id;
	int health;
} Entity;

#define MAX_ENTITY_COUNT 1024

#define X(Class, Sprite, Name) case class_ ## Class : return Sprite; break;
SpriteID get_sprite_id_from_entity_class(EntityClass class) {
	switch(class) {
		ENTITIES
		default: return 0;
	}
}
#undef X

#define X(Class, Sprite, Name) case class_ ## Class : return STR(Name); break;
string get_entity_class_pretty_name(EntityClass class) {
	switch(class) {
		ENTITIES
		default: return STR("nil");
	}
}
#undef X



typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
} World;
World* world = null;

Entity* entity_create() {
	Entity* assigned_entity = null;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* existing_entity = &world->entities[i];
		if (!existing_entity->is_valid) {
			assigned_entity = existing_entity;
			break;
		}
	}

	assert(assigned_entity, "Entity limit reached");
	assigned_entity->is_valid = true;
	return assigned_entity;
}

void entity_draw_sprite(Entity* entity) {
	Sprite* sprite = get_sprite(entity->sprite_id);
	Matrix4 xform = m4_identity;

	xform = m4_translate(xform, v2_as_v3(entity->pos));
	xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0, 0));

	draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);
}

void entity_destroy(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

void ent_setup_player(Entity* entity) {
	entity->class = class_player;
	entity->sprite_id = SPRITE_player;
	entity->visible = true;
}

int entry(int argc, char **argv) {
	
	window.title = STR("Survival");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 400;
	window.clear_color = hex_to_rgba(0x111115ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	Gfx_Font* font = load_font_from_disk(STR("C:/Windows/Fonts/arial.ttf"), get_heap_allocator());

	// load sprites with X macro
	#define X(ID, Path) sprites[SPRITE_ ## ID] = (Sprite){ .image = load_image_from_disk(STR(Path), get_heap_allocator())};
	SPRITES
	#undef X

	Entity* ent_player = entity_create();
	ent_setup_player(ent_player);

	float zoom = 5;
	Vector2 camera_pos = v2_zero;

	f64 last_time = os_get_current_time_in_seconds();
	f32 count_second = 0;
	int32 frame_counter = 0;
	string last_fps = STR("0");
	while (!window.should_close) {
		reset_temporary_storage();
		
		f64 now = os_get_current_time_in_seconds();
		f64 delta_t = now - last_time;
		last_time = now;
		os_update();
		update_timers(delta_t);

		// player movement
		{
			Vector2 input = v2_zero;

			if (is_key_down('A')) {
				input.x -= 1.0;
			}
			if (is_key_down('D')) {
				input.x += 1.0;
			}
			if (is_key_down('S')) {
				input.y -= 1.0;
			}
			if (is_key_down('W')) {
				input.y += 1.0;
			}
			input = v2_normalize(input);

			ent_player->pos = v2_add(ent_player->pos, v2_mulf(input, 100 * delta_t));
		}

		bool tool_used_this_frame = false;
		if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
			tool_used_this_frame = try_start_timer(TIMER_tool_cooldown);
		}

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
		//draw_frame.enable_z_sorting = true;

		// camera
		{
			Vector2 cam_target = ent_player->pos;
			animate_v2(&camera_pos, cam_target, delta_t, 30.0);

			draw_frame.view = m4_identity;
			draw_frame.view = m4_translate(draw_frame.view, v2_as_v3(camera_pos));
			draw_frame.view = m4_scale(draw_frame.view, v3(1.0/zoom, 1.0/zoom, 1.0));
		}

		// draw entities
		{
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* entity = &world->entities[i];
				if (!entity->is_valid)
					continue;

				if (!entity->visible)
					continue;
				
				switch(entity->class) {
					case class_player: {
						entity_draw_sprite(entity);

						Sprite* pickaxe = get_sprite(SPRITE_pickaxe);
						Vector2 size = get_sprite_size(pickaxe);
						Matrix4 xform = m4_identity;

						xform = m4_translate(xform, v2_as_v3(entity->pos));
						xform = m4_translate(xform, v3(0, size.y * 0.5, 0));
						xform = m4_rotate_z(xform, -v2_look_at_radians(entity->pos, get_mouse_world_pos()) + PI32 / 2);

						static f32 tool_distance = 4;

						if (tool_used_this_frame)
							tool_distance = 8;

						if (tool_distance > 4) {
							animate_f32(&tool_distance, 4, delta_t, 10);
						}

						xform = m4_translate(xform, v3(size.x * -0.5, tool_distance, 0));

						draw_image_xform(pickaxe->image, xform, get_sprite_size(pickaxe), COLOR_WHITE);
						break;
					}
					default: {
						entity_draw_sprite(entity);
						break;
					}
				}
			} 
		}

		draw_frame.projection = m4_make_orthographic_projection(0, window.width, 0, window.height, -1, 10);
		draw_frame.view = m4_identity;

		count_second += delta_t;
		frame_counter++;
		if (count_second >= 1.0)
		{
			last_fps = tprintf("%d", frame_counter);
			count_second -= 1.0;
			frame_counter = 0;
		}

		draw_text(font, last_fps, 16, v2(4, window.height - 16), v2_one, COLOR_WHITE);

		gfx_update();
	}

	return 0;
}