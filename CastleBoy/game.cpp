#include "game.h"

#include "menu.h"
#include "map.h"
#include "player.h"
#include "entity.h"
#include "assets.h"

int16_t Game::cameraX;
uint8_t Game::life;
uint16_t Game::timeLeft;
bool Game::hasPlayerDied = false;

namespace
{
const uint8_t* const levels[] = { stage_1_1, stage_1_2, stage_1_4, stage_2_1, stage_2_2, stage_2_3, stage_2_4, stage_3_1, stage_3_4 };

bool finished;
uint8_t levelIndex;
//uint8_t deathCounter = 0;
//uint8_t finishedCounter = 0;

void drawHpBar(int16_t x, int16_t y, uint8_t value, uint8_t max)
{
  ab.fillRect(x, y, 4 * max, 7, BLACK);
  for (uint8_t i = 0; i < max; i++)
  {
    sprites.drawSelfMasked(x + i * 4, y, i < value ? ui_hp_full : ui_hp_empty, 0);
  }
}

}

void Game::reset()
{
  levelIndex = 0;
  life = GAME_STARTING_LIFE;
  Player::hp = PLAYER_MAX_HP;
  Player::knifeCount = 0;
}

void Game::play()
{
  finished = false;
  mainState = STATE_PLAY;
  if (hasPlayerDied)
  {
    Player::hp = PLAYER_MAX_HP;
    hasPlayerDied = false;
  }
  Entities::init();
  Map::init(levels[levelIndex]);
  cameraX = 0;
}

void Game::loop()
{
  // debug
#ifdef DEBUG_CHEAT
  if (ab.pressed(A_BUTTON) && ab.pressed(B_BUTTON) && ab.justPressed(DOWN_BUTTON))
  {
    play();
    return;
  }

  if (ab.pressed(A_BUTTON) && ab.pressed(B_BUTTON) && ab.justPressed(UP_BUTTON))
  {
    if (Map::boss != NULL)
    {
      Menu::notifyStageFinished();
      finished = true;
    }
    else
    {
      ++levelIndex;
      play();
    }
    return;
  }

  if (ab.pressed(A_BUTTON) && ab.pressed(B_BUTTON) && ab.justPressed(LEFT_BUTTON))
  {
    // TODOplay(stage_test);
    return;
  }
#endif

  // update
  Player::update();
  Entities::update();

  if (!finished)
  {
    if (timeLeft > 0)
    {
      --timeLeft;
    }
  
    // finished: exit from left
    if (Player::pos.x - 4 /*normalHitbox.x*/ > Map::width * TILE_WIDTH)
    {
      ++levelIndex;
      play();
    }
    // finished: boss killed
    else if (Map::boss != NULL && Map::boss->hp == 0)
    {
      Menu::notifyStageFinished();
      finished = true;
    }

    //  // finished: update counter
    //  if (finishedCounter > 0)
    //  {
    //    if (--finishedCounter == 0)
    //    {
    //      if (++levelIndex == STAGE_MAX)
    //      {
    //        mainState = STATE_GAME_FINISHED;
    //      }
    //      else
    //      {
    //        if (Map::boss != NULL)
    //        {
    //          ++stage;
    //          Menu::showStageIntro();
    //        }
    //        else
    //        {
    //          play();
    //        }
    //      }
    //    }
    //  }

    // finished: check if player is alive
    else if (!Player::alive || timeLeft == 0)
    {
      Player::knifeCount = 0;
      if (timeLeft == 0)
      {
        life = 0;
      }
      else
      {
        --life;
      }
      Menu::notifyPlayerDied();
      finished = true;

      //deathCounter = 160;
      //Menu::playMusic();
      //sound.tone(NOTE_G3, 100, NOTE_G2, 150, NOTE_G1, 350);
    }
  }

  //  // game over: update counter
  //  if (deathCounter > 0)
  //  {
  //    if (--deathCounter == 0)
  //    {
  //      if (life == 0)
  //      {
  //        mainState = STATE_GAME_OVER;
  //      }
  //      else
  //      {
  //        hasPlayerDied = true;
  //        play();
  //      }
  //    }
  //  }

  // update camera
  if (Player::pos.x < cameraX + CAMERA_LEFT_BUFFER)
  {
    cameraX = Player::pos.x - CAMERA_LEFT_BUFFER;
    if (cameraX < 0) cameraX = 0;
  }
  else if (Player::pos.x > cameraX + 128 - CAMERA_RIGHT_BUFFER)
  {
    cameraX = Player::pos.x - 128 + CAMERA_RIGHT_BUFFER;
    if (cameraX > Map::width * TILE_WIDTH - 128) cameraX = Map::width * TILE_WIDTH - 128;
  }

  // draw: parralax
  if (Map::showBackground)
  {
    int16_t backgroundOffset = cameraX / 28; // FIXME properly calculate parralax unless all maps have same width
    sprites.drawOverwrite(16 - backgroundOffset, 4, background_mountain, 0);
  }

  // draw: main
  Map::draw();
  Entities::draw();
  Player::draw();

  // ui: hp
  drawHpBar(0, 0, Player::hp, PLAYER_MAX_HP);

  // ui: knife count
  ab.fillRect(54, 0, 13, 7, BLACK);
  sprites.drawSelfMasked(55, 0, ui_knife_count, 0);

  Util::drawNumber(68, 0, Player::knifeCount, ALIGN_LEFT);

  // ui: time left
  Util::drawNumber(128, 0, timeLeft / FPS, ALIGN_RIGHT);

  // ui: boss
  if (Map::boss != NULL)
  {
    drawHpBar(40, 58, Map::boss->hp, BOSS_MAX_HP);
  }
}

bool Game::moveY(Vec& pos, int8_t dy, const Box& hitbox, bool collideToEntity)
{
  if (dy != 0)
  {
    int8_t sign = dy > 0 ? 1 : -1;
    while (dy != 0)
    {
      if (Map::collide(pos.x, pos.y + sign, hitbox) || (collideToEntity && Entities::moveCollide(pos.x, pos.y + sign, hitbox)))
      {
        return true;
      }
      pos.y += sign;
      dy -= sign;
    }
  }

  return false;
}
