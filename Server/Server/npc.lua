myid = 99999;

function set_uid(x)
   myid = x;
end

function event_player_move(player)
   npc_type = API_get_type(myid);
   npc_range = API_LV2_range(myid, player);
   if (true == npc_range) then
      if (npc_type == 2) then
         API_move_npc(myid, player);
      end
   end
end


function event_player_attack(player)
   API_attack_npc(myid, player);
end