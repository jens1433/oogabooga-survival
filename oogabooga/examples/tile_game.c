

#define X_TILE_COUNT 128
#define Y_TILE_COUNT 128
#define TILE_COUNT (Y_TILE_COUNT * X_TILE_COUNT)

#define TILE_WIDTH 64
#define TILE_HEIGHT 64

#define WORLD_WIDTH  (X_TILE_COUNT * TILE_WIDTH)
#define WORLD_HEIGHT (Y_TILE_COUNT * TILE_HEIGHT)

#define Z_LAYER_TILE_GRID -1
#define Z_LAYER_TILE_LAYER_BASE 100

#define Z_LAYER_EDITOR_GUI 200

#define MAX_LAYERS 6


typedef struct Tile_Layer {
	Gfx_Image *tile_images[TILE_COUNT];
} Tile_Layer;
Tile_Layer tile_layers[MAX_LAYERS] = {0};

u64 get_tile_index(s32 x, s32 y) {
	return y * X_TILE_COUNT + x;
}
u64 get_tile_index_from_pos(Vector2 p) {
	s32 x = (s32)((p.x/WORLD_WIDTH)*X_TILE_COUNT);
	s32 y = (s32)((p.y/WORLD_HEIGHT)*Y_TILE_COUNT);
	return get_tile_index(x, y);
}

typedef enum {
	APP_STATE_EDITING,
	APP_STATE_PLAYING
} App_State;

f32 delta_time = 0;
App_State app_state = APP_STATE_EDITING;

s64 current_tile_layer = 0;

Matrix4 camera_view;

Gfx_Image *hammer_image;

void update_editor();
void update_game();

int entry(int argc, char **argv) {
	
	window.title = STR("Tile game");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x6495EDff);
	
	camera_view = m4_translate(m4_scalar(1.0), v3(WORLD_WIDTH / 2, WORLD_HEIGHT / 2, 0));

	hammer_image = load_image_from_disk(STR("oogabooga/examples/hammer.png"), get_heap_allocator());
	assert(hammer_image);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		
		draw_frame.projection = m4_make_orthographic_projection(window.pixel_width * -0.5, window.pixel_width * 0.5, window.pixel_height * -0.5, window.pixel_height * 0.5, -1, 10);
		draw_frame.enable_z_sorting = true;
		
		float64 now = os_get_current_time_in_seconds();
		delta_time = (float32)(now - last_time);
		last_time = now;
		
		if (app_state == APP_STATE_EDITING) {
			update_editor();
		} else if (app_state == APP_STATE_PLAYING) {
			update_game();
		}

		draw_rect(v2(0, 0), v2(100, 100), v4(1,0,0,1));
		
		os_update(); 
		gfx_update();
	}

	return 0;
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

void update_editor() {
	
	const float32 cam_move_speed = 400.0;
	Vector2 cam_move_axis = v2(0, 0);
	if (is_key_down('A')) {
		cam_move_axis.x -= 1.0;
	}
	if (is_key_down('D')) {
		cam_move_axis.x += 1.0;
	}
	if (is_key_down('S')) {
		cam_move_axis.y -= 1.0;
	}
	if (is_key_down('W')) {
		cam_move_axis.y += 1.0;
	}

	
	
	Vector2 cam_move = v2_mulf(cam_move_axis, delta_time * cam_move_speed);
	camera_view = m4_translate(camera_view, v3(v2_expand(cam_move), 0));
	draw_frame.view = camera_view;
	
	Vector2 bottom_left = screen_to_world(v2(0, 0));
	Vector2 top_right   = screen_to_world(v2(window.width, window.height));
	
	Vector2 origin = v2(0, 0);
	int first_visible_tile_x = ((origin.x + bottom_left.x)/WORLD_WIDTH) *X_TILE_COUNT;
	int first_visible_tile_y = ((origin.y + bottom_left.y)/WORLD_HEIGHT)*Y_TILE_COUNT;
	int last_visible_tile_x  = ((origin.x + top_right.x  )/WORLD_WIDTH) *X_TILE_COUNT;
	int last_visible_tile_y  = ((origin.y + top_right.y  )/WORLD_HEIGHT)*Y_TILE_COUNT;

	int left_edge = -X_TILE_COUNT / 2;
	left_edge = 0;
	int right_edge = X_TILE_COUNT / 2;
	right_edge = X_TILE_COUNT;
	int top_edge = Y_TILE_COUNT / 2;
	top_edge = Y_TILE_COUNT;
	int bottom_edge = -Y_TILE_COUNT / 2;
	bottom_edge = 0;
	first_visible_tile_x = clamp(first_visible_tile_x, left_edge, right_edge);
	log_info("first visible x %d", first_visible_tile_x);
	last_visible_tile_x = clamp(last_visible_tile_x, left_edge, right_edge);
	log_info("first visible y %d", last_visible_tile_x);
	first_visible_tile_y = clamp(first_visible_tile_y, bottom_edge, top_edge);
	log_info("last visible x %d", first_visible_tile_y);
	last_visible_tile_y = clamp(last_visible_tile_y, bottom_edge, top_edge);
	log_info("last visible y %d", last_visible_tile_y);
	
	// Visualize empty tile grid & react to mouse
	push_z_layer(Z_LAYER_TILE_GRID);
	
	int selected_x = 0;
	int selected_y = 0;
	for (s32 tile_x = first_visible_tile_x; tile_x <= last_visible_tile_x; tile_x++) {
		for (s32 tile_y = first_visible_tile_y; tile_y <=last_visible_tile_y; tile_y++) {
			bool variation = (tile_x%2==0 && tile_y%2==1) || (tile_x%2==1 && tile_y%2==0);
			
			Vector2 pos = v2_add(origin, v2(tile_x*TILE_WIDTH, tile_y*TILE_HEIGHT));
			//log_verbose("%d, %d", tile_x, pos.y);
			draw_rect(pos, v2(TILE_WIDTH, TILE_HEIGHT), variation?v4(.27,.27,.27,1):v4(.3,.3,.3,1));
			
			push_z_layer(Z_LAYER_EDITOR_GUI);
			
			float left = pos.x;
			float bottom = pos.y;
			float right = left + TILE_WIDTH;
			float top = bottom + TILE_HEIGHT;
			Vector2 m = get_mouse_world_pos();
			bool hovered = m.x >= left && m.x < right && m.y >= bottom && m.y < top;
			
			if (hovered) {
				selected_x = tile_x;
				selected_y = tile_y;
				draw_rect(pos, v2(TILE_WIDTH, TILE_HEIGHT), v4(0, 0, 0, 0.3));
			}
			
			pop_z_layer();
		}
	}

	if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
		Tile_Layer* layer = &tile_layers[0];
		layer->tile_images[get_tile_index(selected_x, selected_y)] = hammer_image;
	}

	pop_z_layer();
	
	for (int i = 0; i < MAX_LAYERS; i++) {
		Tile_Layer *layer = &tile_layers[i];
		push_z_layer(Z_LAYER_TILE_LAYER_BASE + i);
		for (s32 tile_x = first_visible_tile_x; tile_x <= last_visible_tile_x; tile_x += 1) {
			for (s32 tile_y = first_visible_tile_y; tile_y <=last_visible_tile_y; tile_y += 1) {
				u64 tile_index = get_tile_index(tile_x, tile_y);
				Gfx_Image *img = layer->tile_images[tile_index];
				if (img) {
					log_verbose("drawing img at %d", tile_index);
					Vector2 pos = v2_add(origin, v2(tile_x*TILE_WIDTH, tile_y*TILE_HEIGHT));
					draw_image(img, pos, v2(TILE_WIDTH, TILE_HEIGHT), COLOR_WHITE);
				}
			}
		}
		pop_z_layer();
	}
}
void update_game() {
	
}