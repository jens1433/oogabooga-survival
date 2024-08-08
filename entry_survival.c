
#define m4_identity m4_scalar(1.0)

#define v2_as_v3(v) v3((v).x, (v).y, 0)

#define v2_zero v2(0, 0)

#define v2_one v2(1, 1)

#define Z_PLAYER 100
#define Z_ITEM 50

#define Z_UI_BASE 1000
#define Z_UI_INVENTORY Z_UI_BASE + 10

#define DEBUG_ENTITY_SHOW_ID 0

Vector2 get_random_v2_in_range_i(Range2f range) {
	return v2(get_random_int_in_range(range.min.x, range.max.x), get_random_int_in_range(range.min.y, range.max.y));
}

bool range2f_overlap(Range2f self, Range2f other) {
	Vector2 self_size = range2f_size(self);
	Vector2 other_size = range2f_size(other);
	if (self_size.x < other_size.x) {
		Vector2 minmax_x = v2(self.min.x, self.max.x);
		self.min.x = other.min.x;
		self.max.x = other.max.x;
		other.min.x = minmax_x.x;
		other.max.x = minmax_x.y;
	}
	if (self_size.y < other_size.y) {
		Vector2 minmax_x = v2(self.min.y, self.max.y);
		self.min.y = other.min.y;
		self.max.y = other.max.y;
		other.min.y = minmax_x.x;
		other.max.y = minmax_x.y;
	}

	bool x = (other.min.x >= self.min.x && other.min.x <= self.max.x) || (other.max.x >= self.min.x && other.max.x <= self.max.x);
	bool y = (other.min.y >= self.min.y && other.min.y <= self.max.y) || (other.max.y >= self.min.y && other.max.y <= self.max.y);
	return x && y;
}

bool int_array_add_unique(int* array, int item, int index)
{
	for (int i = 0; i < index; i++)
	{
		if (array[i] == item)
		{
			return false;
		}
	}
	array[index] = item;

	return true;
}

inline f32 v2_look_at_radians(Vector2 from, Vector2 to)
{
	return atan2f(to.y - from.y, to.x - from.x);
}

inline Vector2 m4_as_v2(Matrix4 m)
{
	return m4_transform(m, v4(0, 0, 0, 1)).xy;
}

Vector2 screen_to_world(Vector2 screen)
{
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

Vector2 get_mouse_world_pos()
{
	return screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y));
}

bool almost_equals(float a, float b, float epsilon)
{
	return fabs(a - b) <= epsilon;
}

bool animate_f32(f32* value, f32 target, f32 delta_t, f32 rate)
{
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true;
	}
	return false;
}

f32 animate_f32_sin_breathe(float time, float rate) {
	return (sin(time * rate) + 1.0) / 2.0;
}

void animate_v2(Vector2* value, Vector2 target, f32 delta_t, f32 rate)
{
	animate_f32(&(value->x), target.x, delta_t, rate);
	animate_f32(&(value->y), target.y, delta_t, rate);
}

#define TIMERS \
	X(tool_cooldown, 0.2)

#define X(ID, Length) TIMER_##ID,
typedef enum TimerID
{
	TIMER_nil,
	TIMERS
	TIMER_MAX
} TimerID;
#undef X

typedef struct Timer
{
	f32 length;
	f32 progress;
	bool finished;
} Timer;

#define X(ID, Length) (Timer){Length, 0, true},
Timer timers[] = {
	(Timer) {
0, 0, true
},
TIMERS };
#undef X

void update_timers()
{
	for (int i = 0; i < TIMER_MAX; i++)
	{
		Timer* timer = &timers[i];
		if (timer->finished)
			continue;

		timer->progress += context.extra.delta_t;
		if (timer->progress >= timer->length)
			timer->finished = true;
	}
}

Timer* get_timer(TimerID timerID)
{
	if (timerID < TIMER_MAX)
	{
		return &timers[timerID];
	}

	return &timers[0];
}

bool try_start_timer(TimerID timerID)
{
	Timer* timer = get_timer(timerID);
	if (timer->finished)
	{
		timer->finished = false;
		timer->progress = 0;
		return true;
	}
	return false;
}

#define SPRITES                                   \
	X(player, "res/sprites/character.png")        \
	X(pickaxe, "res/sprites/pickaxe.png")         \
	X(rock_small, "res/sprites/rock-small.png")   \
	X(rock_medium, "res/sprites/rock-medium.png") \
	X(rock_large, "res/sprites/rock-large.png")   \
	X(ui_inventory_slot, "res/sprites/item-slot.png")

typedef struct Sprite
{
	Gfx_Image* image;
} Sprite;

#define X(ID, Path) SPRITE_##ID,
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITES
	SPRITE_MAX,
} SpriteID;
#undef X

Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id)
{
	if (id < SPRITE_MAX)
	{
		return &sprites[id];
	}

	return &sprites[0];
}

Vector2 get_sprite_size(Sprite* sprite)
{
	return (Vector2) { sprite->image->width, sprite->image->height };
}

#define ENTITIES                  \
	X(player, "Player")           \
	X(rock_small, "Small Rock")   \
	X(rock_medium, "Medium Rock") \
	X(rock_large, "Large Rock")   \
	X(item_stone, "Stone")

#define X(Class, Name) class_##Class,
typedef enum EntityClass
{
	class_nil,
	ENTITIES
	CLASS_MAX
} EntityClass;
#undef X

typedef struct Entity
{
	bool is_valid;
	EntityClass class;
	bool is_item;
	int identifier;
	Vector2 pos;
	Vector2 size;
	bool visible;
	SpriteID sprite_id;
	int health;
	struct Chunk* chunk;
} Entity;

typedef Entity* EntityPtr;

#define MAX_ENTITY_COUNT 1024

Range2f entity_bounds(EntityPtr entity)
{
	Range2f bounds = range2f_make_bottom_center(entity->size);
	bounds = range2f_shift(bounds, entity->pos);
	return bounds;
}

SpriteID get_sprite_id_from_entity_class(EntityClass class)
{
	switch (class)
	{
	case class_item_stone:
		return SPRITE_rock_small;
	default:
		return 0;
	}
}

#define X(Class, Name)    \
	case class_##Class:   \
		return STR(Name); \
		break;
string get_entity_class_pretty_name(EntityClass class)
{
	switch (class)
	{
		ENTITIES
	default:
		return STR("nil");
	}
}
#undef X

typedef struct PulledItem {
	Entity* item;
	f32 progress;
} PulledItem;

#define MAX_PULLED_ITEMS 16

typedef struct ItemSlot
{
	EntityClass class;
	int amount;
} ItemSlot;

#define MAX_ITEM_SLOTS 16

#define UI_INVENTORY_ROW_LENGTH 8

typedef struct PlayerData
{
	ItemSlot item_slots[MAX_ITEM_SLOTS];
	f32 tool_angle;
	PulledItem pulled_items[MAX_PULLED_ITEMS];
} PlayerData;
PlayerData* player_data = null;

void inventory_add_item(EntityClass item_class, int amount) {
	for (int i = 0; i < MAX_ITEM_SLOTS; i++) {
		ItemSlot* slot = &player_data->item_slots[i];
		if (slot->class == class_nil) {
			slot->class = item_class;
			slot->amount = amount;
			break;
		}
		if (slot->class == item_class) {
			slot->amount += amount;
			break;
		}
	}
}

#define WORLD_SIZE_IN_CHUNKS 32
#define CHUNK_COUNT WORLD_SIZE_IN_CHUNKS * WORLD_SIZE_IN_CHUNKS
#define CHUNK_SIZE 128
#define ROCKS_PER_CHUNK 10

#define MAX_ENTITIES_IN_CHUNK 1024

inline bool is_valid_chunk_index(int index) {
	return index >= 0 && index < CHUNK_COUNT;
}

typedef struct Chunk
{
	Range2f chunk_bounds;
	Entity* entities_in_chunk[MAX_ENTITIES_IN_CHUNK];
	int entity_count;
	bool generated;
	u64 seed;
} Chunk;

void chunk_remove_entity(Chunk* chunk, EntityPtr entity)
{
	for (int i = 0; i < MAX_ENTITIES_IN_CHUNK; i++)
	{
		Entity* ent = entity->chunk->entities_in_chunk[i];
		if (ent == entity)
		{
			chunk->entity_count--;
			chunk->entities_in_chunk[i] = chunk->entities_in_chunk[chunk->entity_count];
			chunk->entities_in_chunk[chunk->entity_count] = null;
			break;
		}
	}
	entity->chunk = null;
}

void chunk_add_entity(Chunk* chunk, EntityPtr entity)
{
	chunk->entities_in_chunk[chunk->entity_count] = entity;
	chunk->entity_count++;
	entity->chunk = chunk;
}

EntityPtr chunk_find_entity_at_pos(Chunk* chunk, Vector2 pos)
{
	for (int i = 0; i < chunk->entity_count; i++) {
		EntityPtr ent = chunk->entities_in_chunk[i];
		if (range2f_contains(entity_bounds(ent), pos)) {
			return ent;
		}
	}
	return null;
}

typedef enum UIState {
	UI_None,
	UI_Inventory
} UIState;

typedef struct UIStyle {
	Gfx_Font* default_font;
} UIStyle;
UIStyle* style = null;

typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
	Chunk chunks[CHUNK_COUNT];
	UIState ui_state;
} World;
World* world = null;

int pos_to_chunk_index(Vector2 position)
{
	f32 size = CHUNK_SIZE * WORLD_SIZE_IN_CHUNKS;
	f32 half_size = CHUNK_SIZE * WORLD_SIZE_IN_CHUNKS / 2.0;
	int x = (half_size + position.x) / CHUNK_SIZE;
	int y = (half_size + position.y) / CHUNK_SIZE;

	int index = y * WORLD_SIZE_IN_CHUNKS + x;
	//assert(index >= 0 && index < CHUNK_COUNT, "Chunk index %d out of bounds", index);

	if (is_valid_chunk_index(index))
		return index;
	else
		return -1;
}

typedef struct HitResult
{
	bool blocking_hit;
	Entity* hit_entity;
	Vector2 position;
} HitResult;

HitResult line_cast(Vector2 from, Vector2 to, f32 step_size) {
	HitResult result = (HitResult){ 0 };

	Vector2 ray = v2_sub(to, from);
	Vector2 direction = v2_normalize(ray);
	f32 length = v2_length(ray);
	for (float step = 0; step <= length; step += step_size) {
		Vector2 test_pos = v2_add(from, v2_mulf(direction, step));
		int chunk_index = pos_to_chunk_index(test_pos);
		if (chunk_index < 0)
			continue;

		Chunk* chunk = &world->chunks[chunk_index];
		Entity* entity = chunk_find_entity_at_pos(chunk, test_pos);
		if (entity) {
			result.blocking_hit = true;
			result.hit_entity = entity;
			result.position = test_pos;
			return result;
		}
	}

	return result;
}

bool is_pos_in_chunk(Chunk* chunk, Vector2 pos)
{
	return range2f_contains(chunk->chunk_bounds, pos);
}

EntityPtr entity_create()
{
	Entity* assigned_entity = null;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity* existing_entity = &world->entities[i];
		if (!existing_entity->is_valid)
		{
			assigned_entity = existing_entity;
			break;
		}
	}

	assert(assigned_entity, "Entity limit reached");

	assigned_entity->is_valid = true;
	static int next_entity_identifier = 0;
	assigned_entity->identifier = next_entity_identifier++;
	return assigned_entity;
}

void entity_draw_sprite(EntityPtr entity)
{
	Sprite* sprite = get_sprite(entity->sprite_id);
	Matrix4 xform = m4_identity;

	xform = m4_translate(xform, v2_as_v3(entity->pos));
	xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0, 0));

	draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

	if (DEBUG_ENTITY_SHOW_ID)
	{
		push_z_layer(100);
		string s = tprintf("id: %d", entity->identifier);
		Gfx_Font* font = context.extra.debug_font;
		Gfx_Text_Metrics text = measure_text(font, s, 16, v2(0.2, 0.2));
		draw_text_xform(font, s, 16, m4_translate(xform, v3(0, -text.visual_size.y, 0)), v2(0.2, 0.2), COLOR_BLACK);
		pop_z_layer();
	}

}

void entity_draw_as_item(EntityPtr entity, f32 time)
{
	Sprite* sprite = get_sprite(entity->sprite_id);
	Vector2 size = get_sprite_size(sprite);
	Matrix4 xform = m4_identity;

	f32 anim_offset = animate_f32_sin_breathe(time, 5);

	xform = m4_translate(xform, v2_as_v3(entity->pos));
	xform = m4_translate(xform, v3(size.x * -0.5, 0, 0));
	Matrix4 xform_anim = m4_translate(xform, v3(0, anim_offset, 0));

	push_z_layer(Z_ITEM);
	draw_image_xform(sprite->image, xform_anim, size, COLOR_WHITE);
	pop_z_layer();

	xform = m4_translate(xform, v3(0, -size.y / 2, 0));

	push_z_layer(Z_ITEM - 1);
	draw_circle_xform(xform, v2(size.x, size.x * 0.4), v4(0, 0, 0, 0.3));
	pop_z_layer();
}

void entity_destroy(EntityPtr entity)
{
	if (entity->chunk)
		chunk_remove_entity(entity->chunk, entity);
	memset(entity, 0, sizeof(Entity));
}

void entity_setup_player(EntityPtr entity)
{
	entity->class = class_player;
	entity->sprite_id = SPRITE_player;
	entity->visible = true;
}

void entity_setup_rock_small(EntityPtr entity)
{
	entity->class = class_rock_small;
	entity->sprite_id = SPRITE_rock_small;
	entity->visible = true;
	entity->size = get_sprite_size(get_sprite(SPRITE_rock_small));
	entity->health = 1;
}

void entity_setup_rock_medium(EntityPtr entity)
{
	entity->class = class_rock_medium;
	entity->sprite_id = SPRITE_rock_medium;
	entity->visible = true;
	entity->size = get_sprite_size(get_sprite(SPRITE_rock_medium));
	entity->health = 2;
}

void entity_setup_rock_large(EntityPtr entity)
{
	entity->class = class_rock_large;
	entity->sprite_id = SPRITE_rock_large;
	entity->visible = true;
	entity->size = get_sprite_size(get_sprite(SPRITE_rock_large));
	entity->health = 3;
}

void entity_setup_item(EntityPtr entity, EntityClass class)
{
	entity->class = class;
	entity->is_item = true;
	entity->sprite_id = get_sprite_id_from_entity_class(class);
	entity->visible = true;
}

void entity_update_chunk(EntityPtr entity)
{
	if (entity->chunk != null)
	{
		if (is_pos_in_chunk(entity->chunk, entity->pos))
			return;

		chunk_remove_entity(entity->chunk, entity);
	}

	int new_chunk_index = pos_to_chunk_index(entity->pos);
	if (new_chunk_index < 0)
		return;
	Chunk* new_chunk = &world->chunks[new_chunk_index];
	chunk_add_entity(new_chunk, entity);
}

void entity_die(EntityPtr entity)
{
	EntityClass item_to_spawn = class_nil;
	int amount_to_spawn = 0;
	switch (entity->class) {
	case class_rock_small:
		item_to_spawn = class_item_stone;
		amount_to_spawn = 1;
		break;
	case class_rock_medium:
		item_to_spawn = class_item_stone;
		amount_to_spawn = 2;
		break;
	case class_rock_large:
		item_to_spawn = class_item_stone;
		amount_to_spawn = 3;
		break;
	}

	for (int i = 0; i < amount_to_spawn; i++) {
		if (item_to_spawn != class_nil) {
			EntityPtr item = entity_create();
			entity_setup_item(item, item_to_spawn);
			item->pos = get_random_v2_in_range_i(entity_bounds(entity));
			entity_update_chunk(item);
		}
	}

	entity_destroy(entity);
}

void entity_damage(EntityPtr entity, int amount)
{
	entity->health = max(entity->health - amount, 0);

	if (entity->health == 0) {
		entity_die(entity);
	}
}

void chunk_generate(Chunk* chunk)
{
	chunk->seed = seed_for_random;
	for (int i = 0; i < ROCKS_PER_CHUNK; i++)
	{
		EntityPtr ent = entity_create();
		int type = get_random_int_in_range(0, 2);
		switch (type)
		{
		case 0:
			entity_setup_rock_small(ent);
			break;
		case 1:
			entity_setup_rock_medium(ent);
			break;
		case 2:
			entity_setup_rock_large(ent);
			break;
		}
		Range2f sized_bounds = chunk->chunk_bounds;
		sized_bounds.min = v2_add(sized_bounds.min, v2(ent->size.x / 2, 0));
		sized_bounds.max = v2_sub(sized_bounds.max, v2(ent->size.x / 2, ent->size.y));
		bool overlaps = false;
		do {
			overlaps = false;
			ent->pos = get_random_v2_in_range_i(sized_bounds);

			for (int j = 0; j < chunk->entity_count; j++) {
				Entity* other_ent = chunk->entities_in_chunk[j];
				if (ent == other_ent)
					continue;

				if (range2f_overlap(entity_bounds(ent), entity_bounds(other_ent))) {
					overlaps = true;
					break;
				}
			}
		} while (overlaps);

		chunk_add_entity(chunk, ent);
	}

	chunk->generated = true;
}

void chunk_regenerate(Chunk* chunk)
{
	int offset = 0;
	int count = chunk->entity_count;
	for (int i = 0; i < count; i++) {
		Entity* ent = chunk->entities_in_chunk[offset];
		if (ent->class != class_player)
			entity_destroy(ent);
		else
			offset++;
	}
	seed_for_random = chunk->seed;
	chunk_generate(chunk);
}

void find_visible_chunks(int* visible_chunks, int* visible_chunks_found)
{
	Vector2 bottom_left = screen_to_world(v2(0, 0));
	Vector2 top_right = screen_to_world(v2(window.width, window.height));
	Vector2 scaled_size = range2f_size(range2f_make(bottom_left, top_right));

	int steps_x = 1;
	if (scaled_size.x >= CHUNK_SIZE)
	{
		steps_x = ceilf(scaled_size.x / CHUNK_SIZE);
	}
	int step_size_x = scaled_size.x / steps_x;

	int steps_y = 1;
	if (scaled_size.y >= CHUNK_SIZE)
	{
		steps_y = ceilf(scaled_size.y / CHUNK_SIZE);
	}
	int step_size_y = scaled_size.y / steps_y;

	for (int x = 0; x <= steps_x; x++) {
		int step_x = x * step_size_x;
		for (int y = 0; y <= steps_y; y++) {
			int step_y = y * step_size_y;
			Vector2 pos = v2_add(bottom_left, v2(step_x, step_y));
			int chunk_index = pos_to_chunk_index(pos);
			if (chunk_index < 0)
				continue;
			*visible_chunks_found += int_array_add_unique(visible_chunks, chunk_index, *visible_chunks_found);
		}
	}
}

typedef struct Test {
	int value;
	float v;
} Test;

EntityPtr* find_entities_in_radius(Vector2 origin, f32 radius, int max_entities) {
	int* chunks_to_search;
	growing_array_init(&chunks_to_search, sizeof(int), get_temporary_allocator());

	growing_array_add_unique_int(&chunks_to_search, pos_to_chunk_index(v2_add(origin, v2(radius, -radius))));
	growing_array_add_unique_int(&chunks_to_search, pos_to_chunk_index(v2_add(origin, v2(radius, radius))));
	growing_array_add_unique_int(&chunks_to_search, pos_to_chunk_index(v2_add(origin, v2(-radius, -radius))));
	growing_array_add_unique_int(&chunks_to_search, pos_to_chunk_index(v2_add(origin, v2(-radius, radius))));

	EntityPtr* entities;
	growing_array_init(&entities, sizeof(EntityPtr), get_temporary_allocator());

	int chunk_count = growing_array_get_valid_count(chunks_to_search);
	for (int i = 0; i < chunk_count; i++) {
		int chunk_index = chunks_to_search[i];
		if (chunk_index < 0)
			continue;
		Chunk* chunk = &world->chunks[chunk_index];
		if (!chunk)
			continue;

		for (int j = 0; j < chunk->entity_count; j++) {
			EntityPtr ent = chunk->entities_in_chunk[j];
			f32 length = v2_length(v2_sub(ent->pos, origin));
			if (length < radius) {
				growing_array_add(&entities, &ent);
			}
		}
	}
	growing_array_deinit(&chunks_to_search);

	return entities;
}

int entry(int argc, char** argv)
{

	window.title = STR("Survival");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 400;
	window.clear_color = hex_to_rgba(0x111115ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));
	player_data = alloc(get_heap_allocator(), sizeof(PlayerData));
	memset(player_data, 0, sizeof(PlayerData));
	style = alloc(get_heap_allocator(), sizeof(UIStyle));
	memset(style, 0, sizeof(UIStyle));


	context.extra.debug_font = load_font_from_disk(STR("C:/Windows/Fonts/arial.ttf"), get_heap_allocator());
	style->default_font = context.extra.debug_font;

	// load sprites with X macro
#define X(ID, Path) sprites[SPRITE_##ID] = (Sprite){.image = load_image_from_disk(STR(Path), get_heap_allocator())};
	SPRITES;
#undef X

	// generate chunk bounds and spawn chunks
	{
		int i = 0;
		int half = WORLD_SIZE_IN_CHUNKS / 2;
		for (int y = -half; y < half; y++) {
			for (int x = -half; x < half; x++) {
				Range2f bounds = range2f_make(v2_zero, v2(CHUNK_SIZE, CHUNK_SIZE));
				bounds = range2f_shift(bounds, v2(x * CHUNK_SIZE, y * CHUNK_SIZE));
				world->chunks[i].chunk_bounds = bounds;

				// generate spawn chunks
				if (abs(x) <= 2 && abs(y) <= 2) {
					chunk_generate(&world->chunks[i]);
				}

				i++;
			}
		}
	}

	EntityPtr ent_player = entity_create();
	entity_setup_player(ent_player);
	bool tool_used_this_frame = false;

	float zoom = 5;
	Vector2 camera_pos = v2_zero;

	f64 last_time = os_get_current_time_in_seconds();
	f32 count_second = 0;
	int32 frame_counter = 0;
	string last_fps = alloc_string(get_heap_allocator(), 1);
	memset(last_fps.data, 0, last_fps.count);
	while (!window.should_close)
	{
		reset_temporary_storage();

		f64 now = os_get_current_time_in_seconds();
		f64 delta_t = now - last_time;
		context.extra.delta_t = delta_t;
		last_time = now;
		os_update();
		update_timers();

		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
		draw_frame.enable_z_sorting = true;

		// camera
		{
			Vector2 cam_target = ent_player->pos;
			animate_v2(&camera_pos, cam_target, delta_t, 30.0);

			draw_frame.view = m4_identity;
			draw_frame.view = m4_translate(draw_frame.view, v2_as_v3(camera_pos));
			draw_frame.view = m4_scale(draw_frame.view, v3(1.0 / zoom, 1.0 / zoom, 1.0));
		}

		{
			// Inventory
			if (is_key_just_pressed('E')) {
				if (world->ui_state == UI_Inventory) {
					world->ui_state = UI_None;
				}
				else {
					world->ui_state = UI_Inventory;
				}
			}
		}

		// gameplay inputs
		if (world->ui_state == UI_None)
		{
			// player movement
			{
				Vector2 input = v2_zero;

				if (is_key_down('A'))
				{
					input.x -= 1.0;
				}
				if (is_key_down('D'))
				{
					input.x += 1.0;
				}
				if (is_key_down('S'))
				{
					input.y -= 1.0;
				}
				if (is_key_down('W'))
				{
					input.y += 1.0;
				}
				input = v2_normalize(input);

				ent_player->pos = v2_add(ent_player->pos, v2_mulf(input, 100 * delta_t));

				entity_update_chunk(ent_player);
			}

			// use tool
			{
				player_data->tool_angle = -v2_look_at_radians(ent_player->pos, get_mouse_world_pos()) + PI32 / 2;

				tool_used_this_frame = false;
				if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
				{
					tool_used_this_frame = try_start_timer(TIMER_tool_cooldown);
					if (tool_used_this_frame) {
						Vector2 size = get_sprite_size(get_sprite(SPRITE_pickaxe));
						Matrix4 xform = m4_identity;

						xform = m4_translate(xform, v2_as_v3(ent_player->pos));
						xform = m4_translate(xform, v3(0, size.y * 0.5, 0));
						xform = m4_rotate_z(xform, player_data->tool_angle);
						xform = m4_translate(xform, v3(0, size.y + 4, 0));
						Vector2 from = m4_as_v2(xform);
						xform = m4_translate(xform, v3(0, 4, 0));
						Vector2 to = m4_as_v2(xform);

						HitResult result = line_cast(from, to, 0.1);
						if (result.blocking_hit) {
							entity_damage(result.hit_entity, 1);
						}
					}
				}
			}
		}


		// item pull
		{
			EntityPtr* items = find_entities_in_radius(ent_player->pos, 20, 64);
			int item_count_in_radius = growing_array_get_valid_count(items);
			for (int i = 0; i < item_count_in_radius; i++) {
				EntityPtr item = items[i];
				if (!item->is_item)
					continue;
				for (int j = 0; j < MAX_PULLED_ITEMS; j++) {
					PulledItem* p_item = &player_data->pulled_items[j];
					if (p_item->item == item)
						break;
					if (!p_item->item) {
						p_item->item = item;
						p_item->progress = 0;
						break;
					}
				}
			}

			for (int i = 0; i < MAX_PULLED_ITEMS; i++) {
				PulledItem* p_item = &player_data->pulled_items[i];
				if (p_item->item) {
					p_item->progress += context.extra.delta_t * 0.05;
					Entity* item = p_item->item;
					Vector2 delta = v2_sub(ent_player->pos, item->pos);
					item->pos = v2_add(item->pos, v2_mulf(delta, p_item->progress));
					if (v2_length(delta) < 4) {
						inventory_add_item(item->class, 1);
						entity_destroy(item);
						p_item->item = null;
					}
					else {
						entity_update_chunk(item);
					}
				}
			}
		}


		// draw entities
		{
			int visible_chunk_count = 0;
			int visible_chunks[64];
			find_visible_chunks(visible_chunks, &visible_chunk_count);

			static f32 item_anim = 0;
			item_anim += delta_t;

			for (int i = 0; i < visible_chunk_count; i++)
			{
				Chunk* chunk = &world->chunks[visible_chunks[i]];
				if (!chunk->generated)
					chunk_generate(chunk);

				for (int j = 0; j < chunk->entity_count; j++)
				{
					Entity* entity = chunk->entities_in_chunk[j];
					if (!entity->is_valid)
						continue;

					if (!entity->visible)
						continue;

					switch (entity->class)
					{
					case class_player:
					{
						push_z_layer(Z_PLAYER);

						entity_draw_sprite(entity);

						Sprite* pickaxe = get_sprite(SPRITE_pickaxe);
						Vector2 size = get_sprite_size(pickaxe);
						Matrix4 xform = m4_identity;

						xform = m4_translate(xform, v2_as_v3(entity->pos));
						xform = m4_translate(xform, v3(0, size.y * 0.5, 0));
						xform = m4_rotate_z(xform, player_data->tool_angle);

						static f32 tool_distance = 4;

						if (tool_used_this_frame)
							tool_distance = 8;

						if (tool_distance > 4)
						{
							animate_f32(&tool_distance, 4, delta_t, 10);
						}

						xform = m4_translate(xform, v3(size.x * -0.5, tool_distance, 0));

						draw_image_xform(pickaxe->image, xform, size, COLOR_WHITE);

						pop_z_layer();
						break;
					}
					default:
					{
						if (entity->is_item)
							entity_draw_as_item(entity, item_anim);
						else
							entity_draw_sprite(entity);
						break;
					}
					}
				}
			}

			push_z_layer(-1);
			for (int i = 0; i < CHUNK_COUNT; i++) {
				Range2f bounds = world->chunks[i].chunk_bounds;
				int x = bounds.min.x / CHUNK_SIZE;
				int y = bounds.min.y / CHUNK_SIZE;
				bool variation = (abs(x % 2) == 0 && abs(y % 2) == 1) || (abs(x % 2) == 1 && abs(y % 2) == 0);
				draw_rect(bounds.min, range2f_size(bounds), variation ? v4(1, 1, 1, 0.1) : v4(1, 1, 1, 0.2));
			}
			pop_z_layer();
		}

		
		push_z_layer(Z_UI_BASE);

		draw_frame.projection = m4_make_orthographic_projection(0, window.width, 0, window.height, -1, 10);
		draw_frame.view = m4_make_scale(v3(0.2, 0.2, 1));

		Vector2 scaled_window = v2_mulf(v2(window.width, window.height), 0.2);

		if (world->ui_state > 0) {

			draw_rect(v2_zero, scaled_window, v4(0, 0, 0, 0.5));

			switch (world->ui_state)
			{
			case UI_Inventory:
			{
				const int slot_spacing = 5;
				Sprite* slot_sprite = get_sprite(SPRITE_ui_inventory_slot);
				Vector2 slot_size = get_sprite_size(slot_sprite);
				int slots_width = slot_size.x * UI_INVENTORY_ROW_LENGTH + slot_spacing * (UI_INVENTORY_ROW_LENGTH - 1);
				int base_y = scaled_window.y / 2;

				for (int i = 0; i < MAX_ITEM_SLOTS; i++)
				{
					int x = i % UI_INVENTORY_ROW_LENGTH;
					int y = i / UI_INVENTORY_ROW_LENGTH;

					Vector2 slot_left_bottom = v2(scaled_window.x / 2 - slots_width / 2 + x * slot_size.x + x * slot_spacing, base_y - y * slot_size.y - y * slot_spacing);

					draw_image(slot_sprite->image, slot_left_bottom, slot_size, COLOR_WHITE);

					ItemSlot* slot_data = &player_data->item_slots[i];
					if (slot_data->class != class_nil) {
						Vector2 slot_center = v2_add(slot_left_bottom, v2_mulf(slot_size, 0.5));
						Sprite* item_sprite = get_sprite(get_sprite_id_from_entity_class(slot_data->class));
						Vector2 item_size = get_sprite_size(item_sprite);
						draw_image(item_sprite->image, v2_sub(slot_center, v2_mulf(item_size, 0.5)), item_size, COLOR_WHITE);


					}
				}
			}
			break;
			}

		}

		count_second += delta_t;
		frame_counter++;
		if (count_second >= 1.0)
		{
			dealloc_string(get_heap_allocator(), last_fps);
			last_fps = sprintf(get_heap_allocator(), "%d", frame_counter);
			count_second -= 1.0;
			frame_counter = 0;
		}

		draw_text(context.extra.debug_font, last_fps, 24, v2(4, scaled_window.y - 4), v2(0.2, 0.2), COLOR_WHITE);

		pop_z_layer();

		gfx_update();
	}

	return 0;
}