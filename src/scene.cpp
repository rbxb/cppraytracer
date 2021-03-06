#include "scene.h"
#include <algorithm>
#include <math.h>

scene::scene()
{
	v = view{
		100.0f,
		70.0f,
	};
	objects = std::vector<object*>{};
	lights = std::vector<light*>{};
	ambient = v3{ 0,0,0 };
	fogDistance = 1000;
}

scene::scene(view view_, std::vector<object*> objects_, std::vector<light*> lights_, v3 ambient_, float fogDistance_)
{
	v = view_;
	objects = objects_;
	lights = lights_;
	ambient = min(ambient_, 1);
	fogDistance = fogDistance_;
}

view scene::getView()
{
	return v;
}

v3 scene::cast(v3 orig, v3 dir, char count, float retained)
{
	v3 c = ambient;
	if (count == 0 || retained < 0.01f) return c;
	count--;
	c = lighting(orig, dir);
	c = pow(c, 40);
	float closestDistance = -1;
	object * closestObject = nullptr;
	for (object * o : objects) {
		float d = o->intersect(orig, dir);
		if (d > 0 && (d < closestDistance || closestDistance < 0)) {
			closestDistance = d;
			closestObject = o;
		}
	}
	if (closestObject != nullptr) {
		orig = dir * closestDistance + orig;
		hit h = closestObject->at(orig);
		v3 hover = h.surfacen * 0.001f + orig;
		v3 lightColor = lighting(hover, h.mappedn);

		if (h.albedo > 0) {
			v3 refld = reflect(dir, h.mappedn);
			float albedo = std::min(h.albedo + h.albedo * fresnel(dir, refld), 1.0f);
			v3 reflColor = cast(hover, refld, count, retained * albedo);
			lightColor = min(reflColor * albedo + lightColor  * (1 - albedo), 1);
		}

		if (h.opacity < 1) {
			v3 sink;
			if (dir * h.surfacen < 0) {
				sink = -h.surfacen * 0.001f + orig;
			}
			else {
				sink = h.surfacen * 0.001f + orig;
			}
			v3 refrd = refract(dir, h.mappedn, h.refraction);
			v3 refrColor = cast(sink, refrd, count, retained * (1 - h.opacity));
			lightColor = min(lightColor * h.opacity + refrColor * (1 - h.opacity), 1);
		}

		c = c + multiply(h.c, lightColor);

		//fog
		float fog = std::min(closestDistance / fogDistance, 1.0f);
		c = c * (1 - fog) + ambient * fog;
	} else {
		c = c + ambient;
	}
	return min(c, 1);
}

v3 scene::lighting(v3 orig, v3 dir)
{
	v3 c = ambient;
	for (light * l : lights) {
		v3 ldir = l->dir(orig);
		float ld = l->d(orig);
		if (!simpleCast(orig, ldir, ld)) {
			float m = std::max(dir * ldir, 0.0f);
			c = c + l->getColor() * m;
		}
	}
	return min(c, 1);
}

bool scene::simpleCast(v3 orig, v3 dir, float ld)
{
	for (object * o : objects) {
		float d = o->intersect(orig, dir);
		if (d > 0 && (ld < 0 || d < ld)) {
			return true;
		}
	}
	return false;
}
