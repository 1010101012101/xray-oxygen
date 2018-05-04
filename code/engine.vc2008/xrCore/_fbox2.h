#ifndef __FBOX2
#define __FBOX2

template <class T>
class _box2 {
public:
	typedef T			TYPE;
	typedef _box2<T>	Self;
	typedef Self&		SelfRef;
	typedef const Self&	SelfCRef;
	typedef _vector2<T>	Tvector;
public:
	union {
		struct {
			Tvector min;
			Tvector max;
		};
		struct {
			T x1, y1;
			T x2, y2;
		};
	};

	inline 	SelfRef	set(const Tvector &_min, const Tvector &_max) { min.set(_min);	max.set(_max);	return *this; };
	inline	SelfRef	set(T x1, T y1, T x2, T y2) { min.set(x1, y1);	max.set(x2, y2); return *this; };
	inline	SelfRef	set(SelfCRef b) { min.set(b.min);	max.set(b.max); return *this; };

	inline	SelfRef	null() { min.set(0.f, 0.f);	max.set(0.f, 0.f); return *this; };
	inline	SelfRef	identity() { min.set(-0.5, -0.5, -0.5);	max.set(0.5, 0.5, 0.5);	return *this; };
	inline	SelfRef	invalidate() { min.set(type_max<T>, type_max<T>); max.set(type_min<T>, type_min<T>); return *this; }

	inline	SelfRef	shrink(T s) { min.add(s); max.sub(s); return *this; };
	inline	SelfRef	shrink(const Tvector& s) { min.add(s); max.sub(s); return *this; };
	inline	SelfRef	grow(T s) { min.sub(s); max.add(s); return *this; };
	inline	SelfRef	grow(const Tvector& s) { min.sub(s); max.add(s); return *this; };

	inline	SelfRef	add(const Tvector &p) { min.add(p); max.add(p); return *this; };
	inline	SelfRef	offset(const Tvector &p) { min.add(p); max.add(p); return *this; };
	inline	SelfRef	add(SelfCRef b, const Tvector &p) { min.add(b.min, p); max.add(b.max, p);	return *this; };

	inline	BOOL contains(T x, T y) { return (x >= x1) && (x <= x2) && (y >= y1) && (y <= y2); };
	inline	BOOL contains(const Tvector &p) { return contains(p.x, p.y); };
	inline	BOOL contains(SelfCRef b) { return contains(b.min) && contains(b.max); };

	inline	BOOL similar(SelfCRef b) { return min.similar(b.min) && max.similar(b.max); };

	inline	SelfRef	modify(const Tvector &p) { min.min(p); max.max(p); return *this; }
	inline	SelfRef	merge(SelfCRef b) { modify(b.min); modify(b.max); return *this; };
	inline	SelfRef	merge(SelfCRef b1, SelfCRef b2) { invalidate(); merge(b1); merge(b2);	return *this; }

	inline	void getsize(Tvector& R)	const { R.sub(max, min); };
	inline	void getradius(Tvector& R)	const { getsize(R); R.mul(0.5f); };
	inline	T getradius() const { Tvector R; getsize(R); R.mul(0.5f);	return R.magnitude(); };

	inline	void	getcenter(Tvector& C)	const
	{
		C.x = (min.x + max.x) * 0.5f;
		C.y = (min.y + max.y) * 0.5f;
	};

	inline	void	getsphere(Tvector &C, T &R) const
	{
		getcenter(C);
		R = C.distance_to(max);
	};

	// Detects if this box intersect other
	inline	BOOL	intersect(SelfCRef box)
	{
		if (max.x < box.min.x)
			return FALSE;

		if (max.y < box.min.y)
			return FALSE;

		if (min.x > box.max.x)
			return FALSE;

		if (min.y > box.max.y)
			return FALSE;

		return TRUE;
	};

	// Make's this box valid AABB
	inline SelfRef sort()
	{
		T tmp;
		if (min.x > max.x) { tmp = min.x; min.x = max.x; max.x = tmp; }
		if (min.y > max.y) { tmp = min.y; min.y = max.y; max.y = tmp; }
		return *this;
	};

	// Does the vector3 intersects box
	inline BOOL Pick(const Tvector& start, const Tvector& dir)
	{
		T		alpha, xt, yt;
		Tvector rvmin, rvmax;

		rvmin.sub(min, start);
		rvmax.sub(max, start);

		if (!fis_zero(dir.x))
		{
			alpha = rvmin.x / dir.x;
			yt = alpha * dir.y;
			if (yt >= rvmin.y && yt <= rvmax.y)
				return true;
			alpha = rvmax.x / dir.x;
			yt = alpha * dir.y;
			if (yt >= rvmin.y && yt <= rvmax.y)
				return true;
		}

		if (!fis_zero(dir.y))
		{
			alpha = rvmin.y / dir.y;
			xt = alpha * dir.x;
			if (xt >= rvmin.x && xt <= rvmax.x)
				return true;
			alpha = rvmax.y / dir.y;
			xt = alpha * dir.x;
			if (xt >= rvmin.x && xt <= rvmax.x)
				return true;
		}
		return false;
	};

	inline BOOL pick_exact(const Tvector& start, const Tvector& dir)
	{
		T		alpha, xt, yt;
		Tvector rvmin, rvmax;

		rvmin.sub(min, start);
		rvmax.sub(max, start);

		if (_abs(dir.x) != 0)
		{
			alpha = rvmin.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
				return true;

			alpha = rvmax.x / dir.x;
			yt = alpha * dir.y;

			if (yt >= rvmin.y - EPS && yt <= rvmax.y + EPS)
				return true;
		}

		if (_abs(dir.y) != 0)
		{
			alpha = rvmin.y / dir.y;
			xt = alpha * dir.x;
			if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
				return true;
			alpha = rvmax.y / dir.y;
			xt = alpha * dir.x;
			if (xt >= rvmin.x - EPS && xt <= rvmax.x + EPS)
				return true;
		}
		return false;
	};

	inline u32& IR(T &x) { return (u32&)x; }
	inline BOOL Pick2(const Tvector& origin, const Tvector& dir, Tvector& coord) {
		BOOL Inside = TRUE;
		Tvector	MaxT;
		MaxT.x = MaxT.y = -1.0f;

		// Find candidate planes.
		{
			if (origin[0] < min[0]) {
				coord[0] = min[0];
				Inside = FALSE;
				if (IR(dir[0]))	MaxT[0] = (min[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
			}
			else if (origin[0] > max[0]) {
				coord[0] = max[0];
				Inside = FALSE;
				if (IR(dir[0]))	MaxT[0] = (max[0] - origin[0]) / dir[0]; // Calculate T distances to candidate planes
			}
		}
		{
			if (origin[1] < min[1]) {
				coord[1] = min[1];
				Inside = FALSE;
				if (IR(dir[1]))	MaxT[1] = (min[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
			}
			else if (origin[1] > max[1]) {
				coord[1] = max[1];
				Inside = FALSE;
				if (IR(dir[1]))	MaxT[1] = (max[1] - origin[1]) / dir[1]; // Calculate T distances to candidate planes
			}
		}

		// Ray origin inside bounding box
		if (Inside)
		{
			coord = origin;
			return true;
		}

		// Get largest of the maxT's for final choice of intersection
		u32 WhichPlane = 0;

		if (MaxT[1] > MaxT[0])
			WhichPlane = 1;

		// Check final candidate actually inside box
		if (IR(MaxT[WhichPlane]) & 0x80000000)
			return false;

		if (0 == WhichPlane)
		{
			// 1
			coord[1] = origin[1] + MaxT[0] * dir[1];

			if ((coord[1] < min[1]) || (coord[1] > max[1]))
				return false;

			return true;
		}
		else {
			// 0
			coord[0] = origin[0] + MaxT[1] * dir[0];

			if ((coord[0] < min[0]) || (coord[0] > max[0]))
				return false;

			return true;
		}
	}

	inline void getpoint(int index, Tvector& result) {
		switch (index) {
		case 0:
			result.set(min.x, min.y);
			break;
		case 1:
			result.set(min.x, min.y);
			break;
		case 2:
			result.set(max.x, min.y);
			break;
		case 3:
			result.set(max.x, min.y);
			break;
		default:
			result.set(0.f, 0.f);
			break;
		}
	};

	inline void getpoints(Tvector* result)
	{
		result[0].set(min.x, min.y);
		result[1].set(min.x, min.y);
		result[2].set(max.x, min.y);
		result[3].set(max.x, min.y);
	};
};

typedef _box2<float> Fbox2;
typedef _box2<double> Dbox2;

template <class T>
BOOL _valid(const _box2<T>& c) { return _valid(c.min) && _valid(c.max); }

#endif
