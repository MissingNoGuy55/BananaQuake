/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h

#pragma once	// Missi: more QCC crap (12/3/2022)

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

extern vec3_t vec3_origin;
extern	int nanmask;

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
#define RAD2DEG( a ) ( a * 180.0F) / M_PI;

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define DoublePrecisionDotProduct(x,y) ((double)(x)[0]*(y)[0]+(double)(x)[1]*(y)[1]+(double)(x)[2]*(y)[2])
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}

void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

vec_t _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

int VectorCompare (vec3_t v1, vec3_t v2);

template<typename T>
T Lerp(T x, T y, T alpha);

vec_t Length (vec3_t v);
void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);
float VectorNormalize (vec3_t v);		// returns vector length
void VectorInverse (vec3_t v);
void VectorScale (vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod (double numer, double denom, int *quotient,
		int *rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor (int i1, int i2);

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mplane_t* p);
float	anglemod(float a);

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
vec_t VectorLength(vec3_t v);

void RotateVector(vec3_t& input, vec3_t& forward, vec3_t& right, vec3_t& up, float angle);
void RotationFromVector(vec3_t& distanceBetweenPoints, vec3_t& output);

//================================
// Missi: 3D vector class
// So I don't have to keep using the annoying vec3_t functions (7/22/2024)
//================================
class Vector3
{
public:
	Vector3()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	Vector3(vec3_t& vec)
	{
		x = vec[0];
		y = vec[1];
		z = vec[2];
	}
	Vector3(const vec3_t& vec)
	{
		x = vec[0];
		y = vec[1];
		z = vec[2];
	}
	Vector3(Vector3& vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}
	Vector3(const Vector3& vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}
	Vector3(float x1, float y1, float z1)
	{
		x = x1;
		y = y1;
		z = z1;
	}

	void RotateAlongAxis(Vector3& forward, Vector3& right, Vector3& up, float angle);
	vec3_t& ToVec3_t();

	Vector3 Rotation(vec3_t dist);

	Vector3 operator=(const Vector3& other);
	Vector3 operator=(const vec3_t& other);

	Vector3& operator+(const Vector3& other);
	Vector3& operator+(const float& other);
	
	Vector3& operator-(const Vector3& other);
	Vector3& operator-(const float& other);
	
	Vector3& operator*(const Vector3& other);
	Vector3& operator*(const float& other);
	
	Vector3& operator*=(const Vector3& other);
	Vector3& operator*=(const float& other);

	Vector3& operator/(const Vector3& other);
	Vector3& operator/(const float& other);

	const bool operator==(const Vector3& vec1);
	const bool operator!=(const Vector3& vec1);

	float x, y, z;
};

extern Vector3 vec_null;

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

template<typename T>
T Lerp(T x, T y, T alpha)
{
    return x * (1.0 - alpha) + (y * alpha);
}
