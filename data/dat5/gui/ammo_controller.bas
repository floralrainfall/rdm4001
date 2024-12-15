gui.setvalue("HUDAmmo", "ammo_primary", xl.realtostr(gui.getinvalue("HUDAmmo", "primary")))

secondary = gui.getinvalue("HUDAmmo", "secondary")
if secondary = 0.0 then
   gui.setcolor("HUDAmmo", "ammo_secondary", 0.65, 0.65, 0.65)
else
   gui.setcolor("HUDAmmo", "ammo_secondary", 0.84, 0.48, 0.72)
endif
gui.setvalue("HUDAmmo", "ammo_secondary", xl.realtostr(secondary))