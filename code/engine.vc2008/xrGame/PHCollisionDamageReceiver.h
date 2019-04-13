#pragma once
#include "../xrphysics/icollisiondamagereceiver.h"

class CPhysicsShellHolder;

class CPHCollisionDamageReceiver : public ICollisionDamageReceiver
{
	using SControledBone = std::pair<u16, float>;
	using DAMAGE_CONTROLED_BONES_V = xr_vector<SControledBone>;
	using DAMAGE_BONES_I = DAMAGE_CONTROLED_BONES_V::iterator;
	struct SFind { u16 id; SFind(u16 _id) { id = _id; }; bool operator () (const SControledBone& cb) { return cb.first == id; } };
	DAMAGE_CONTROLED_BONES_V m_controled_bones;

protected:
	virtual CPhysicsShellHolder* PPhysicsShellHolder() = 0;
	void						Init();

	void						Clear();
private:
	void						BoneInsert(u16 id, float k);

	DAMAGE_BONES_I				FindBone(u16 id)
	{
		return std::find_if(m_controled_bones.begin(), m_controled_bones.end(), SFind(id));
	}
	void						CollisionHit(u16 source_id, u16 bone_id, float power, const Fvector &dir, Fvector &pos);
};