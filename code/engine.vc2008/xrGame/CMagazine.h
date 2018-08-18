#pragma once
#include "WeaponAmmo.h"

class CMagazine
{
public:
	CMagazine();
	~CMagazine();

	xr_vector<shared_str>	m_ammoTypes;

	// Multitype ammo support
	xr_vector<CCartridge>	m_magazine;
	CCartridge				m_DefaultCartridge;
	float					m_fCurrentCartirdgeDisp;
	CWeaponAmmo*			m_pCurrentAmmo;
	u8						m_ammoType;
	u8						m_u8TracerColorID;
	bool					m_bHasTracers;

	// �������� ��� ������
	float					GetMagazineWeight(const decltype(m_magazine)& mag) const;

protected:

	mutable int				m_iAmmoCurrentTotal;

	// ������� ���������� �������� � �������� ������
	int iAmmoElapsed;

	// ��������������� �������� � ��������
	int	iMagazineSize;



};

