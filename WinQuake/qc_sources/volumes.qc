//
// Missi: BananaQuake fog volume stuff
//

.string fog, fog_start, fog_end, fog_r, fog_g, fog_b, fog_density, fog_lerp_time;

.float world_fog_start, world_fog_end, world_fog_r, world_fog_g, world_fog_b, world_fog_density, world_fog_lerp_time;

void() func_fog_volume_touch =
{
	if (other.classname != "player")
		return;
	
	cvar_set("fog_density_goal", self.fog_density);
	cvar_set("fog_goal_r", self.fog_r);
	cvar_set("fog_goal_g", self.fog_g);
	cvar_set("fog_goal_b", self.fog_b);
	cvar_set("fog_lerp_time", self.fog_lerp_time);
	cvar_set("fog_start_goal", self.fog_start);
	cvar_set("fog_end_goal", self.fog_end);
};

void() func_fog_volume_think =
{	
	if (self.fog_r == ftos(cvar("fog_r")))
	{
		bprint("re-enabling fog volume touch\n");
		self.touch = func_fog_volume_touch;
	}
	
	self.nextthink = time + 0.1;
	self.think = func_fog_volume_think;
};

/*QUAKED func_fog_volume (.5 .5 .5) ? PUSH_ONCE
Changes fog settings when player is in the volume
*/
void() func_fog_volume =
{
	InitTrigger ();
	
	self.world_fog_start = cvar("fog_start");
	self.world_fog_end = cvar("fog_end");
	self.world_fog_density = cvar("fog_density");
	self.world_fog_lerp_time = cvar("fog_lerp_time");
	self.world_fog_r = cvar("fog_r");
	self.world_fog_g = cvar("fog_g");
	self.world_fog_b = cvar("fog_b");
	
	precache_sound ("ambience/windfly.wav");
	self.touch = func_fog_volume_touch;
	self.think = func_fog_volume_think;
};
