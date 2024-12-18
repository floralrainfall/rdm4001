health = gui.getinvalue("HUD", "health")
if health < 15.0 then
   gui.setcolor("HUD", "health", 1.0, 0.0, 0.0)
else
   gui.setcolor("HUD", "health", 0.84, 0.48, 0.72)	
endif
gui.setvalue("HUD", "health", xl.realtostr(health))

armor = gui.getinvalue("HUD", "armor")
if armor = 0.0 then  
   gui.setcolor("HUD", "armor", 0.65, 0.65, 0.65)
else
   gui.setcolor("HUD", "armor", 0.84, 0.48, 0.72)	
endif
gui.setvalue("HUD", "armor", xl.realtostr(armor))