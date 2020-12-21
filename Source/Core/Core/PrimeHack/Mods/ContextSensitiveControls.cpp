#include "Core/PrimeHack/Mods/ContextSensitiveControls.h"

#include "Core/PrimeHack/PrimeUtils.h"
#include "Core/PrimeHack/HackConfig.h"

namespace prime {

void ContextSensitiveControls::run_mod(Game game, Region region) {
  // Always reset the camera lock or it will never be unlocked
  // This is done before the game check just in case somebody quits during a motion puzzle.
  SetLockCamera(false);

  if (game != Game::PRIME_3 && game != Game::PRIME_3_STANDALONE) {
    return;
  }

  u32 obj_list_iterator = 0;
  if (game == Game::PRIME_3_STANDALONE && region == Region::NTSC_U) {
    obj_list_iterator = read32(read32(cplayer_ptr_address) + 0x1018) + 4;
  } else {
    obj_list_iterator = read32(read32(cplayer_ptr_address - 4) + 0x1018) + 4;
  }
  const u32 base = obj_list_iterator;

  for (int i = 0; i < 1024; i++) {
    u32 entity = read32(obj_list_iterator);
    u32 entity_flags = read32(entity + 0x38);

    bool should_process = false;
    if (entity_flags & 0x20000000) {
      should_process = ((entity_flags >> 8) & 0x2000) == 0;
    }
    should_process |= ((entity_flags >> 8) & 0x1000) != 0;

    if (should_process) {
      u32 vf_table = read32(entity);
      u32 vft_func = read32(vf_table + 0xc);

      // Accept function for this specific object type ("RTTI" checking)
      if (vft_func == motion_vtf_address) {
        u32 puzzle_state = read32(entity + 0x14c);

        if (ImprovedMotionControls()) {
          if (puzzle_state == 3) {
            float step = readf32(entity + 0x154);

            if (CheckForward()) {
              step += 0.05f;
            }
            if (CheckBack()) {
              step -= 0.05f;
            }

            writef32(std::clamp(step, 0.f, 1.f), entity + 0x154);
          }
        }

        if (LockCameraInPuzzles()) {
          // if object is active and isn't the ship radio at the start of the game.
          if (puzzle_state > 0 && read32(entity + 0xC) != 0x0c180263) {
            SetLockCamera(true);
          }
        }  
      } else if (vft_func == motion_vtf_address + 0x38) {
        // If the vtf is for rotary, confirm this needs to be controlled
        if (read32(entity + 0x204) == 1) {
          float velocity = 0;

          if (CheckRight())
            velocity = 0.04f;
          if (CheckLeft())
            velocity -= 0.04f;

          writef32(velocity, 0x80004170);
        }
      }
    }

    u16 next_id = read16(obj_list_iterator + 6);
    if (next_id == 0xffff) {
      break;
    }

    obj_list_iterator = (base + next_id * 8);
  }
}

void ContextSensitiveControls::init_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_3:
    if (region == Region::NTSC_U) {
      // Take control of the rotary puzzles
      code_changes.emplace_back(0x801F806C, 0x3D808000);
      code_changes.emplace_back(0x801F8074, 0x618C4170);
      code_changes.emplace_back(0x801F807C, 0xC02C0000);

      cplayer_ptr_address = 0x805c6c6c;
      motion_vtf_address = 0x802e0dac;
    }
    else if (region == Region::PAL) {
      code_changes.emplace_back(0x801F7B4C, 0x3D808000);
      code_changes.emplace_back(0x801F7B54, 0x618C4170);
      code_changes.emplace_back(0x801F7B5C, 0xC02C0000);

      cplayer_ptr_address = 0x805ca0ec;
      motion_vtf_address = 0x802e0a88;
    }
    break;
  case Game::PRIME_3_STANDALONE:
    if (region == Region::NTSC_U) {
      code_changes.emplace_back(0x801fb544, 0x3D808000);
      code_changes.emplace_back(0x801fb54c, 0x618C4170);
      code_changes.emplace_back(0x801fb554, 0xC02C0000);

      cplayer_ptr_address = 0x805c4f98;
      motion_vtf_address = 0x802e2508;
    }
    else if (region == Region::PAL) {
      code_changes.emplace_back(0x801fc5a8, 0x3D808000);
      code_changes.emplace_back(0x801fc5b0, 0x618C4170);
      code_changes.emplace_back(0x801fc5b8, 0xC02C0000);

      cplayer_ptr_address = 0x805c759c;
      motion_vtf_address = 0x802e3be4;
    }
    break;
  default:
    break;
  }
  initialized = true;
}
}