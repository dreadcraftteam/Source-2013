// Tool for convenient testing smth in game
// By DREADCRAFT
//

#include "cbase.h"

#include "string"

#include "in_buttons.h"
#include "con_nprint.h"
#include "vphysics/constraints.h"

#ifndef CLIENT_DLL
#include "EntityDissolve.h"
#include "ndebugoverlay.h"
#include "rope.h"
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

ConVar devtool_mode("devtool_mode", "0", FCVAR_REPLICATED, "Changing devtool modes");
ConVar devtool_notifications("devtool_notifications", "1", FCVAR_REPLICATED, "Enable/Disable dev-tool notifications (very buggy!!!)");

#ifdef CLIENT_DLL
#define CWeaponDevTool C_WeaponDevTool
#endif

class CWeaponDevTool : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponDevTool, CBaseHL2MPCombatWeapon);

	CWeaponDevTool(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	Precache(void);

	void	PrimaryAttack(void);

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);

	bool	Reload(void);

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual int	GetMinBurst()
	{
		return 1;
	}

	virtual int	GetMaxBurst()
	{
		return 3;
	}

	virtual float GetFireRate(void)
	{
		return 0.5f;
	}

	struct physpropinfo
	{
		CHandle<CBaseEntity> handle;
	};

#ifndef CLIENT_DLL
	struct duplicatedata
	{
		string_t className;
		string_t modelName;
		int skin;
		int body;
		float scale;
	};

	duplicatedata m_DupeData;
	bool m_bHasDupe = false;
#endif

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CNetworkVar(float, m_flSoonestPrimaryAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);
	CNetworkVar(int, m_nNumShotsFired);

private:
#ifndef CLIENT_DLL
	CUtlVector<physpropinfo> m_hPhysProps;
	
	bool m_bIsRagdoll;
	
	string_t m_szModelName;
	CHandle<CEntityDissolve> m_pDissolver;

	int m_nModelSkin;
	int m_nModelBody;
#endif // !CLIENT_DLL

	float m_flTargetScale = 1.0f;
	float m_flScaleSpeed = 0.5f;
	CHandle<CBaseEntity> m_hScaledEntity;

	CWeaponDevTool(const CWeaponDevTool&);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponDevTool, DT_WeaponDevTool)

BEGIN_NETWORK_TABLE(CWeaponDevTool, DT_WeaponDevTool)
#ifdef CLIENT_DLL
RecvPropTime(RECVINFO(m_flSoonestPrimaryAttack)),
RecvPropTime(RECVINFO(m_flLastAttackTime)),
RecvPropFloat(RECVINFO(m_flAccuracyPenalty)),
RecvPropInt(RECVINFO(m_nNumShotsFired)),
#else
SendPropTime(SENDINFO(m_flSoonestPrimaryAttack)),
SendPropTime(SENDINFO(m_flLastAttackTime)),
SendPropFloat(SENDINFO(m_flAccuracyPenalty)),
SendPropInt(SENDINFO(m_nNumShotsFired)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponDevTool)
DEFINE_PRED_FIELD(m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_devtool, CWeaponDevTool);
PRECACHE_WEAPON_REGISTER(weapon_devtool);

#ifndef CLIENT_DLL
acttable_t CWeaponDevTool::m_acttable[] =
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};


IMPLEMENT_ACTTABLE(CWeaponDevTool);

#endif

CWeaponDevTool::CWeaponDevTool(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1 = 0;
	m_fMaxRange1 = 0;
	m_fMinRange2 = 0;
	m_fMaxRange2 = 0;

	m_iPrimaryAmmoType = -1;
	m_iSecondaryAmmoType = -1;
}

void CWeaponDevTool::Precache(void)
{
	BaseClass::Precache();
}

#ifndef CLIENT_DLL
void ScaleObject(CBaseEntity* pEnt, float scale)
{
	if (!pEnt)
		return;

	if (pEnt->IsPlayer() || pEnt->IsNPC())
		return;

	CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(pEnt);
	if (!pAnim)
		return;

	pAnim->SetModelScale(scale, 0);

	IPhysicsObject* pPhys = pAnim->VPhysicsGetObject();
	if (!pPhys)
		return;

	Vector pos;
	QAngle ang;
	pPhys->GetPosition(&pos, &ang);

	pPhys->EnableCollisions(false);

	pAnim->VPhysicsDestroyObject();
	if (!pAnim->VPhysicsInitNormal(SOLID_VPHYSICS, 0, false))
		return;

	pPhys = pAnim->VPhysicsGetObject();
	if (pPhys)
	{
		pPhys->SetPosition(pos, ang, false);
		pPhys->Wake();
	}
}
#endif

void CWeaponDevTool::PrimaryAttack()
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	
	if (!pOwner)
		return;

	WeaponSound(SINGLE);
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	QAngle vecAngles(0, GetAbsAngles().y - 90, 0);
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecAiming = pOwner->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector impactPoint = vecSrc + (vecAiming * MAX_TRACE_LENGTH);
	Vector vForward, vRight, vUp;
	pOwner->EyeVectors(&vForward, &vRight, &vUp);
	Vector muzzlePoint = pOwner->Weapon_ShootPosition() + vForward + vRight + vUp;

	trace_t trace;
	UTIL_TraceLine(muzzlePoint, muzzlePoint + vForward * MAX_TRACE_LENGTH, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace);
	
	Vector vecShootOrigin, vecShootDir;
	Vector VPhysicsGetObject;
	vecShootOrigin = pOwner->Weapon_ShootPosition();

	if (trace.fraction == 1.0)
		return;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;

	CBaseEntity* pEnt = trace.m_pEnt;

	switch (devtool_mode.GetInt())
	{
	case 0:
		if (trace.m_pEnt->VPhysicsGetObject() || trace.m_pEnt->IsNPC())
		{
#ifndef CLIENT_DLL
			if (!trace.m_pEnt->IsPlayer())
			{
				m_pDissolver = CEntityDissolve::Create(trace.m_pEnt, STRING(m_szModelName), gpGlobals->curtime, NULL);

				if (devtool_notifications.GetInt() == 1)
					NDebugOverlay::Text(trace.endpos, "Removed!", true, 2.0f);
			}
#endif // !CLIENT_DLL
		}
		break;
	case 1:
#ifndef CLIENT_DLL
		if (trace.fraction != 1.0f && trace.m_pEnt && !trace.m_pEnt->IsPlayer() && !trace.m_pEnt->IsNPC() && trace.m_pEnt->VPhysicsGetObject())
		{
			CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(trace.m_pEnt);
			if (pAnim)
			{
				m_hScaledEntity = pAnim;
				m_flTargetScale = pAnim->GetModelScale();
			}
		}
#endif
		break;
	default:
		Msg("Unknown mode! Use 0 - 3\n");
		break;
	}
}

void CWeaponDevTool::UpdatePenaltyTime(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Check our penalty time decay
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, 1.5f);
	}
}

void CWeaponDevTool::ItemPreFrame(void)
{
	con_nprint_t whiteMessage = { 0 };
	con_nprint_t greenMessage = { 0 };

	whiteMessage.time_to_live = 0.1;
	whiteMessage.color[0] = 1.0f, whiteMessage.color[1] = 1.0f, whiteMessage.color[2] = 1.0f;
	whiteMessage.fixed_width_font = true;

	greenMessage.time_to_live = 0.1;
	greenMessage.color[0] = 0.0f, greenMessage.color[1] = 1.0f, greenMessage.color[2] = 0.0f;
	greenMessage.fixed_width_font = true;

	whiteMessage.index = 0;
	engine->Con_NXPrintf(&whiteMessage, "Developer Tool");
	whiteMessage.index = 1;
	engine->Con_NXPrintf(&whiteMessage, "Press reload to change mode.");
	whiteMessage.index = 3;
	engine->Con_NXPrintf(&whiteMessage, "Dev Tool modes:");

	if (devtool_mode.GetInt() == 0)
	{
		greenMessage.index = 5;
		engine->Con_NXPrintf(&greenMessage, "Object Remover");
		whiteMessage.index = 6;
		engine->Con_NXPrintf(&whiteMessage, "Scaler");
	}
	else if (devtool_mode.GetInt() == 1)
	{
		whiteMessage.index = 5;
		engine->Con_NXPrintf(&whiteMessage, "Object Remover");
		greenMessage.index = 6;
		engine->Con_NXPrintf(&greenMessage, "Scaler");
	}
	else if (devtool_mode.GetInt() == 2)
	{
		whiteMessage.index = 5;
		engine->Con_NXPrintf(&whiteMessage, "Object Remover");
		whiteMessage.index = 6;
		engine->Con_NXPrintf(&whiteMessage, "Scaler");
	}
	else if (devtool_mode.GetInt() == 3)
	{
		whiteMessage.index = 5;
		engine->Con_NXPrintf(&whiteMessage, "Object Remover");
		whiteMessage.index = 6;
		engine->Con_NXPrintf(&whiteMessage, "Scaler");
	}
	else
	{
		whiteMessage.index = 5;
		engine->Con_NXPrintf(&whiteMessage, "Object Remover");
		whiteMessage.index = 6;
		engine->Con_NXPrintf(&whiteMessage, "Scaler");
	}

	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponDevTool::ItemBusyFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponDevTool::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (m_bInReload)
		return;

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;


#ifndef CLIENT_DLL
	if (devtool_mode.GetInt() == 1)
	{
		if (m_hScaledEntity)
		{
			bool bChanged = false;

			if (pOwner->m_nButtons & IN_ATTACK)
			{
				m_flTargetScale += m_flScaleSpeed * gpGlobals->frametime;
				bChanged = true;
			}

			if (pOwner->m_nButtons & IN_ATTACK2)
			{
				m_flTargetScale -= m_flScaleSpeed * gpGlobals->frametime;
				bChanged = true;
			}

			m_flTargetScale = clamp(m_flTargetScale, 0.1f, 5.0f);

			if (bChanged)
			{
				CBaseAnimating* pAnim = dynamic_cast<CBaseAnimating*>(m_hScaledEntity.Get());
				if (pAnim)
				{
					pAnim->SetModelScale(m_flTargetScale, 0);

					IPhysicsObject* pPhys = pAnim->VPhysicsGetObject();
					if (pPhys)
					{
						Vector pos;
						QAngle ang;
						pPhys->GetPosition(&pos, &ang);

						pPhys->EnableCollisions(false);

						pAnim->VPhysicsDestroyObject();
						if (pAnim->VPhysicsInitNormal(SOLID_VPHYSICS, 0, false))
						{
							pPhys = pAnim->VPhysicsGetObject();
							if (pPhys)
							{
								pPhys->SetPosition(pos, ang, false);
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
#endif

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack < gpGlobals->curtime))
	{
		PrimaryAttack();
	}

	if ((pOwner->m_nButtons & IN_RELOAD) && (m_flNextPrimaryAttack < gpGlobals->curtime))
	{
		if (pOwner->m_afButtonPressed & IN_RELOAD)
		{
#ifndef CLIENT_DLL
			int mode = devtool_mode.GetInt();
			mode = (mode + 1) % 2;
			devtool_mode.SetValue(mode);
#endif

			WeaponSound(RELOAD);
			SendWeaponAnim(ACT_VM_RELOAD);
		}
	}
}

Activity CWeaponDevTool::GetPrimaryAttackActivity(void)
{
	if (m_nNumShotsFired < 1)
		return ACT_VM_PRIMARYATTACK;

	if (m_nNumShotsFired < 2)
		return ACT_VM_RECOIL1;

	if (m_nNumShotsFired < 3)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

bool CWeaponDevTool::Reload(void)
{
	return false;
}