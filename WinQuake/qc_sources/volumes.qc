//
// Missi: BananaQuake fog volume stuff
//

.string fog_start, fog_end, fog_r, fog_g, fog_b, fog_density, fog_lerp_time;

void() func_fog_volume_touch =
{
	if (other.classname != "player")
		return;
	
	cvar_set("fog_density", self.fog_density);
	cvar_set("fog_r", self.fog_r);
	cvar_set("fog_g", self.fog_g);
	cvar_set("fog_b", self.fog_b);
	cvar_set("fog_lerp_time", self.fog_lerp_time);
	cvar_set("fog_start", self.fog_start);
	cvar_set("fog_end", self.fog_end);
};

/*QUAKED func_fog_volume (.5 .5 .5) ? PUSH_ONCE
Changes fog settings when player is in the volume
*/
void() func_fog_volume =
{
	InitTrigger ();
	
	precache_sound ("ambience/windfly.wav");
	self.touch = func_fog_volume_touch;
};