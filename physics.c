#include <stdio.h>
#include <stdlib.h>
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <math.h>
#define g 9.78033
#define GET(type,list,i)	*((type *) list.ArrayPointer + i)
#define ADD(type,list,item) if (list.Length == 0) list.ArrayPointer = (type *) malloc(sizeof(type));\
							else list.ArrayPointer = realloc(list.ArrayPointer, sizeof(type)*(list.Length + 1));\
							*((type *) list.ArrayPointer + list.Length) = item; list.Length++
typedef struct {
	float x;
	float y;
} Point2D;
typedef struct {
	Point2D direction; // don't set directly
	float magnitude; // don't set directly
} Vector2D;
typedef struct {
	Point2D center;
	float r;
} Circle;
typedef struct {
	float mass;
	Circle circle;
	Vector2D acceleration;// don't set directly
	Vector2D velocity; // don't set directly
	float friction;
	ALLEGRO_COLOR color;
} Object;
typedef struct {
	void * ArrayPointer;
	unsigned int Length;
}  List;
typedef struct {
	Object * a;
	Object * b;
	bool status;
} Collision;
List ObjectList;
List Collisions;
void SetVector2D(Vector2D * _ref_vector2d, float x, float y) {
	_ref_vector2d->direction.x = x;
	_ref_vector2d->direction.y = y;
	_ref_vector2d->magnitude = sqrt(pow(x, 2) + pow(y, 2));
}
void SetVector2DMagnitude(Vector2D * _ref_vector2d, float magnitude) {
	if (magnitude < 0) magnitude = 0;
	float unit = magnitude / _ref_vector2d->magnitude;
	_ref_vector2d->magnitude = magnitude;
	if (unit == 0) return;
	_ref_vector2d->direction.x *= unit;
	_ref_vector2d->direction.y *= unit;
}
void AddForce(Object * _ref_object, Vector2D force) {
	float fx = _ref_object->mass * _ref_object->acceleration.direction.x;
	float fy = _ref_object->mass * _ref_object->acceleration.direction.y;
	fx += force.direction.x;
	fy += force.direction.y;
	SetVector2D(&_ref_object->acceleration, fx / _ref_object->mass, fy / _ref_object->mass);
}
void MakeCollision(Collision * collision) {
	Object * a = collision->a;
	Object * b = collision->b;
	float dx = a->circle.center.x - b->circle.center.x;
	float dy = a->circle.center.y - b->circle.center.y;
	float d = sqrt(dx*dx + dy*dy);
	float nx = dx / d;
	float ny = dy / d;
	float p = 2 * (a->velocity.direction.x * nx + a->velocity.direction.y * ny - b->velocity.direction.x * nx - b->velocity.direction.y * ny) / (a->mass + b->mass);
	float a_vx = a->velocity.direction.x - p * a->mass * nx;
	float a_vy = a->velocity.direction.y - p * a->mass * ny;
	float b_vx = b->velocity.direction.x + p * b->mass * nx;
	float b_vy = b->velocity.direction.y + p * b->mass * ny;
	SetVector2D(&a->velocity, a_vx, a_vy);
	SetVector2D(&b->velocity, b_vx, b_vy);
	collision->status = true;
}
void SearchCollisions() {
	for (int i = 0; i < ObjectList.Length; i++) {
		for (int j = i; j < ObjectList.Length; j++) {
			if (i == j) continue;
			Object * oi = GET(Object *,ObjectList,i);
			Object * oj = GET(Object *, ObjectList, j);
			float diffX = oi->circle.center.x - oj->circle.center.x;
			float diffY = oi->circle.center.y - oj->circle.center.y;
			float diff = sqrt(pow(diffX, 2) + pow(diffY, 2));
			Collision * collision;
			bool isExist = false;
			for (int i = 0; i < Collisions.Length; i++) {
				Collision * c = GET(Collision *, Collisions, i);
				if ((c->a == oi && c->b == oj) || (c->a == oj && c->b == oi)) {
					collision = c;
					isExist = true;
				}
			}
			if (!isExist) {
				collision = (Collision *)malloc(sizeof(Collision));
				collision->a = oi;
				collision->b = oj;
				collision->status = false;
				ADD(Collision *, Collisions, collision);
			}
			if (diff <= oi->circle.r + oj->circle.r) { 
				if (!collision->status) MakeCollision(collision); 
			}
			else collision->status = false;
			
		}
	}
}
void Move(Object * _ref_object) {
	float f = _ref_object->mass * _ref_object->acceleration.magnitude;
	float fs = _ref_object->mass * g * _ref_object->friction;
	if (f > fs) {
		f -= fs;
		SetVector2DMagnitude(&_ref_object->acceleration, f / _ref_object->mass);
	}
	else {
		SetVector2DMagnitude(&_ref_object->acceleration, 0);
		SetVector2DMagnitude(&_ref_object->velocity, _ref_object->velocity.magnitude - fs / _ref_object->mass);
	}
	if(_ref_object->acceleration.magnitude > 0)
		SetVector2D(&_ref_object->velocity, _ref_object->velocity.direction.x + _ref_object->acceleration.direction.x, _ref_object->velocity.direction.y + _ref_object->acceleration.direction.y);
	
	if (_ref_object->velocity.magnitude > 0) {
		_ref_object->circle.center.x += _ref_object->velocity.direction.x;
		_ref_object->circle.center.y += _ref_object->velocity.direction.y;
	}

}
Object * NewObject() {
	Object * obj = (Object *)malloc(sizeof(Object));
	obj->acceleration.direction.x = 0;
	obj->acceleration.direction.y = 0;
	obj->acceleration.magnitude = 0;
	obj->circle.center.x = 0;
	obj->circle.center.y = 0;
	obj->circle.r = 0;
	obj->color = al_map_rgb(255,255,255);
	obj->friction = 0;
	obj->mass = 1000;
	obj->velocity.direction.x = 0;
	obj->velocity.direction.y = 0;
	obj->velocity.magnitude = 0;
	ADD(Object *, ObjectList, obj);
	return obj;
}
void DrawObject(Object obj) {
	al_draw_filled_circle(obj.circle.center.x, - obj.circle.center.y, obj.circle.r, obj.color);
}
int main() {
	al_init();
	al_init_primitives_addon();
	ALLEGRO_DISPLAY * display;
	display = al_create_display(1000,800);
	
	ALLEGRO_TIMER * timer;
	timer = al_create_timer(0.04);
	al_start_timer(timer);
	ALLEGRO_EVENT_QUEUE * queue = al_create_event_queue();
	al_register_event_source(queue, al_get_timer_event_source(timer));
	/* EXAMPLE START */
	Object * obj = NewObject();
	SetVector2D(&obj->velocity, 0.5, -0.3);
	SetVector2D(&obj->acceleration, 0.05, -0.03);
	obj->friction = 0.0004;
	obj->circle.center.x = 50;
	obj->circle.center.y = -50;
	obj->circle.r = 10;

	Object * obj2 = NewObject();
	SetVector2D(&obj2->velocity, -0.5, -0.3);
	SetVector2D(&obj2->acceleration, -0.05, -0.03);
	obj2->friction = 0.0002;
	obj2->circle.center.x = 200;
	obj2->circle.center.y = -50;
	obj2->circle.r = 20;
	obj2->color = al_map_rgb(255, 0, 0);
	while (1) {
		ALLEGRO_EVENT e;
		al_wait_for_event(queue, &e);
		if (e.type == ALLEGRO_EVENT_TIMER) {
			al_draw_filled_rectangle(0, 0, 1000, 800, al_map_rgb(111, 111, 111));
			Move(obj);
			Move(obj2);
			DrawObject(*obj);
			DrawObject(*obj2);
			SearchCollisions();
			al_flip_display();
		}
	}
	/* EXAMPLE END */
	
}
