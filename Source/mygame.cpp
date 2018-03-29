/*
 * mygame.cpp: 本檔案儲遊戲本身的class的implementation
 * Copyright (C) 2002-2008 Woei-Kae Chen <wkc@csie.ntut.edu.tw>
 *
 * This file is part of game, a free game development framework for windows.
 *
 * game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * game is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 *   2002-03-04 V3.1
 *          Add codes to demostrate the use of CMovingBitmap::ShowBitmap(CMovingBitmap &).
 *	 2004-03-02 V4.0
 *      1. Add CGameStateInit, CGameStateRun, and CGameStateOver to
 *         demonstrate the use of states.
 *      2. Demo the use of CInteger in CGameStateRun.
 *   2005-09-13
 *      Rewrite the codes for CBall and CEraser.
 *   2005-09-20 V4.2Beta1.
 *   2005-09-29 V4.2Beta2.
 *      1. Add codes to display IDC_GAMECURSOR in GameStateRun.
 *   2006-02-08 V4.2
 *      1. Revise sample screens to display in English only.
 *      2. Add code in CGameStateInit to demo the use of PostQuitMessage().
 *      3. Rename OnInitialUpdate() -> OnInit().
 *      4. Fix the bug that OnBeginState() of GameStateInit is not called.
 *      5. Replace AUDIO_CANYON as AUDIO_NTUT.
 *      6. Add help bitmap to CGameStateRun.
 *   2006-09-09 V4.3
 *      1. Rename Move() and Show() as OnMove and OnShow() to emphasize that they are
 *         event driven.
 *   2006-12-30
 *      1. Bug fix: fix a memory leak problem by replacing PostQuitMessage(0) as
 *         PostMessage(AfxGetMainWnd()->m_hWnd, WM_CLOSE,0,0).
 *   2008-02-15 V4.4
 *      1. Add namespace game_framework.
 *      2. Replace the demonstration of animation as a new bouncing ball.
 *      3. Use ShowInitProgress(percent) to display loading progress. 
 *   2010-03-23 V4.6
 *      1. Demo MP3 support: use lake.mp3 to replace lake.wav.
*/

#include "stdafx.h"
#include "Resource.h"
#include <mmsystem.h>
#include <ddraw.h>
#include "audio.h"
#include "gamelib.h"
#include "mygame.h"
#include <iostream>

namespace game_framework {

/////////////////////////////////////////////////////////////////////////////
// CBall: Ball class
/////////////////////////////////////////////////////////////////////////////

CBall::CBall()
{
	is_alive = true;
	x = y = dx = dy = index = delay_counter = 0;
}

bool CBall::HitEraser(CEraser *eraser)
{
	// 檢測擦子所構成的矩形是否碰到球
	return HitRectangle(eraser->GetX1(), eraser->GetY1(),
		   eraser->GetX2(), eraser->GetY2());
}

bool CBall::HitRectangle(int tx1, int ty1, int tx2, int ty2)
{
	int x1 = x+dx;				// 球的左上角x座標
	int y1 = y+dy;				// 球的左上角y座標
	int x2 = x1 + bmp.Width();	// 球的右下角x座標
	int y2 = y1 + bmp.Height();	// 球的右下角y座標
	//
	// 檢測球的矩形與參數矩形是否有交集
	//
	return (tx2 >= x1 && tx1 <= x2 && ty2 >= y1 && ty1 <= y2);
}

bool CBall::IsAlive()
{
	return is_alive;
}

void CBall::LoadBitmap()
{
	bmp.LoadBitmap(IDB_BALL,RGB(0,0,0));			// 載入球的圖形
	bmp_center.LoadBitmap(IDB_CENTER,RGB(0,0,0));	// 載入球圓心的圖形
}

void CBall::OnMove()
{
	if (!is_alive)
		return;
	delay_counter--;
	if (delay_counter < 0) {
		delay_counter = delay;
		//
		// 計算球向對於圓心的位移量dx, dy
		//
		const int STEPS=18;
		static const int DIFFX[]={35, 32, 26, 17, 6, -6, -17, -26, -32, -34, -32, -26, -17, -6, 6, 17, 26, 32, };
		static const int DIFFY[]={0, 11, 22, 30, 34, 34, 30, 22, 11, 0, -11, -22, -30, -34, -34, -30, -22, -11, };
		index++;
		if (index >= STEPS)
			index = 0;
		dx = DIFFX[index];
		dy = DIFFY[index];
	}
}

void CBall::SetDelay(int d)
{
	delay = d;
}

void CBall::SetIsAlive(bool alive)
{
	is_alive = alive;
}

void CBall::SetXY(int nx, int ny)
{
	x = nx; y = ny;
}

void CBall::OnShow()
{
	if (is_alive) {
		bmp.SetTopLeft(x+dx,y+dy);
		bmp.ShowBitmap();
		bmp_center.SetTopLeft(x,y);
		bmp_center.ShowBitmap();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBouncingBall: BouncingBall class
/////////////////////////////////////////////////////////////////////////////

CBouncingBall::CBouncingBall()
{
	const int INITIAL_VELOCITY = 20;	// 初始上升速度
	const int FLOOR = 400;				// 地板座標
	floor = FLOOR;
	x = 95; y = FLOOR-1;				// y座標比地板高1點(站在地板上)
	rising = true; 
	initial_velocity = INITIAL_VELOCITY;
	velocity = initial_velocity;
}

void CBouncingBall::LoadBitmap()
{
	char *filename[4]={".\\bitmaps\\ball1.bmp",".\\bitmaps\\ball2.bmp",".\\bitmaps\\ball3.bmp",".\\bitmaps\\ball4.bmp"};
	for (int i = 0; i < 4; i++)	// 載入動畫(由4張圖形構成)
		animation.AddBitmap(filename[i], RGB(0,0,0));
}

void CBouncingBall::OnMove()
{
	if (rising) {			// 上升狀態
		if (velocity > 0) {
			y -= velocity;	// 當速度 > 0時，y軸上升(移動velocity個點，velocity的單位為 點/次)
			velocity--;		// 受重力影響，下次的上升速度降低
		} else {
			rising = false; // 當速度 <= 0，上升終止，下次改為下降
			velocity = 1;	// 下降的初速(velocity)為1
		}
	} else {				// 下降狀態
		if (y < floor-1) {  // 當y座標還沒碰到地板
			y += velocity;	// y軸下降(移動velocity個點，velocity的單位為 點/次)
			velocity++;		// 受重力影響，下次的下降速度增加
		} else {
			y = floor - 1;  // 當y座標低於地板，更正為地板上
			rising = true;	// 探底反彈，下次改為上升
			velocity = initial_velocity; // 重設上升初始速度
		}
	}
	animation.OnMove();		// 執行一次animation.OnMove()，animation才會換圖
}

void CBouncingBall::OnShow()
{
	animation.SetTopLeft(x,y);
	animation.OnShow();
}

/////////////////////////////////////////////////////////////////////////////
// CEraser: Eraser class
/////////////////////////////////////////////////////////////////////////////

CEraser::CEraser()
{
	Initialize();
}

int CEraser::GetX1()
{
	return x;
}

int CEraser::GetY1()
{
	return y;
}

int CEraser::GetX2()
{
	return x + animation.Width();
}

int CEraser::GetY2()
{
	return y + animation.Height();
}

void CEraser::Initialize()
{
	const int X_POS = 280;
	const int Y_POS = 400;
	x = X_POS;
	y = Y_POS;
	isMovingLeft = isMovingRight= isMovingUp = isMovingDown = false;
}

void CEraser::LoadBitmap()
{
	animation.AddBitmap(IDB_ERASER1,RGB(255,255,255));
	animation.AddBitmap(IDB_ERASER2,RGB(255,255,255));
	animation.AddBitmap(IDB_ERASER3,RGB(255,255,255));
	animation.AddBitmap(IDB_ERASER2,RGB(255,255,255));
}

void CEraser::OnMove()
{
//	const int STEP_SIZE = 2;
	animation.OnMove();
//	if (isMovingLeft)
//		x -= STEP_SIZE;
//	if (isMovingRight)
//		x += STEP_SIZE;
//	if (isMovingUp)
//		y -= STEP_SIZE;
//	if (isMovingDown)
//		y += STEP_SIZE;
	if (isMovingLeft && x == 1000 + initX)
		x -= 1000;
	if (isMovingRight && x == 0 + initX)
		x += 1000;
}

void CEraser::SetMovingDown(bool flag)
{
	isMovingDown = flag;
}

void CEraser::SetMovingLeft(bool flag)
{
	isMovingLeft = flag;
}

void CEraser::SetMovingRight(bool flag)
{
	isMovingRight = flag;
}

void CEraser::SetMovingUp(bool flag)
{
	isMovingUp = flag;
}

void CEraser::SetXY(int nx, int ny)
{
	x = nx; y = ny;
}

void CEraser::OnShow()
{
	animation.SetTopLeft(x,y);
	animation.OnShow();
}

void CEraser::Initialize(int nx, int ny)
{
	initX = nx;
	x = nx;
	y = ny;
	isMovingLeft = isMovingRight = isMovingUp = isMovingDown = false;
}

void CEraser::LoadBitmap(int IDB_BITMAP)
{
	animation.AddBitmap(IDB_BITMAP, RGB(255, 255, 255));
}

/////////////////////////////////////////////////////////////////////////////
// 這個class為遊戲的遊戲開頭畫面物件
/////////////////////////////////////////////////////////////////////////////

CGameStateInit::CGameStateInit(CGame *g)
: CGameState(g)
{
}

void CGameStateInit::OnInit()
{
	//
	// 當圖很多時，OnInit載入所有的圖要花很多時間。為避免玩遊戲的人
	//     等的不耐煩，遊戲會出現「Loading ...」，顯示Loading的進度。
	//
	ShowInitProgress(0);	// 一開始的loading進度為0%
	//
	// 開始載入資料
	//
	logo.LoadBitmap(IDB_BACKGROUND);
	Sleep(300);				// 放慢，以便看清楚進度，實際遊戲請刪除此Sleep
	//
	// 此OnInit動作會接到CGameStaterRun::OnInit()，所以進度還沒到100%
	//
}

void CGameStateInit::OnBeginState()
{
}

void CGameStateInit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const char KEY_ESC = 27;
	const char KEY_SPACE = ' ';
	if (nChar == KEY_SPACE)
		GotoGameState(GAME_STATE_RUN);						// 切換至GAME_STATE_RUN
	else if (nChar == KEY_ESC)								// Demo 關閉遊戲的方法
		PostMessage(AfxGetMainWnd()->m_hWnd, WM_CLOSE,0,0);	// 關閉遊戲
}

void CGameStateInit::OnLButtonDown(UINT nFlags, CPoint point)
{
	GotoGameState(GAME_STATE_RUN);		// 切換至GAME_STATE_RUN
}

void CGameStateInit::OnShow()
{
	//
	// 貼上logo
	//
	logo.SetTopLeft((SIZE_X - logo.Width())/2, SIZE_Y/8);
	logo.ShowBitmap();
	//
	// Demo螢幕字型的使用，不過開發時請盡量避免直接使用字型，改用CMovingBitmap比較好
	//
	CDC *pDC = CDDraw::GetBackCDC();			// 取得 Back Plain 的 CDC 
	CFont f,*fp;
	f.CreatePointFont(160,"Times New Roman");	// 產生 font f; 160表示16 point的字
	fp=pDC->SelectObject(&f);					// 選用 font f
	pDC->SetBkColor(RGB(0,0,0));
	pDC->SetTextColor(RGB(255,255,0));
	pDC->TextOut(120,220,"Please click mouse or press SPACE to begin.");
	pDC->TextOut(5,395,"Press Ctrl-F to switch in between window mode and full screen mode.");
	if (ENABLE_GAME_PAUSE)
		pDC->TextOut(5,425,"Press Ctrl-Q to pause the Game.");
	pDC->TextOut(5,455,"Press Alt-F4 or ESC to Quit.");
	pDC->SelectObject(fp);						// 放掉 font f (千萬不要漏了放掉)
	CDDraw::ReleaseBackCDC();					// 放掉 Back Plain 的 CDC
}								

/////////////////////////////////////////////////////////////////////////////
// 這個class為遊戲的結束狀態(Game Over)
/////////////////////////////////////////////////////////////////////////////

CGameStateOver::CGameStateOver(CGame *g)
: CGameState(g)
{
}

void CGameStateOver::OnMove()
{
	counter--;
	if (counter < 0)
		GotoGameState(GAME_STATE_INIT);
}

void CGameStateOver::OnBeginState()
{
	counter = 30 * 5; // 5 seconds
}

void CGameStateOver::OnInit()
{
	//
	// 當圖很多時，OnInit載入所有的圖要花很多時間。為避免玩遊戲的人
	//     等的不耐煩，遊戲會出現「Loading ...」，顯示Loading的進度。
	//
	ShowInitProgress(66);	// 接個前一個狀態的進度，此處進度視為66%
	//
	// 開始載入資料
	//
	Sleep(300);				// 放慢，以便看清楚進度，實際遊戲請刪除此Sleep
	//
	// 最終進度為100%
	//
	ShowInitProgress(100);
}

void CGameStateOver::OnShow()
{
	CDC *pDC = CDDraw::GetBackCDC();			// 取得 Back Plain 的 CDC 
	CFont f,*fp;
	f.CreatePointFont(160,"Times New Roman");	// 產生 font f; 160表示16 point的字
	fp=pDC->SelectObject(&f);					// 選用 font f
	pDC->SetBkColor(RGB(0,0,0));
	pDC->SetTextColor(RGB(255,255,0));
	char str[80];								// Demo 數字對字串的轉換
	sprintf(str, "Game Over ! (%d)", counter / 30);
	pDC->TextOut(240,210,str);
	pDC->SelectObject(fp);						// 放掉 font f (千萬不要漏了放掉)
	CDDraw::ReleaseBackCDC();					// 放掉 Back Plain 的 CDC
	
}

/////////////////////////////////////////////////////////////////////////////
// 這個class為遊戲的遊戲執行物件，主要的遊戲程式都在這裡
/////////////////////////////////////////////////////////////////////////////

CGameStateRun::CGameStateRun(CGame *g)
: CGameState(g), NUMBALLS(28)
{
	ball = new CBall [NUMBALLS];
	picX = picY = 0;
}

CGameStateRun::~CGameStateRun()
{
	delete [] ball;
}

void CGameStateRun::InitMap()
{
	map0.Initialize(0, 0);

	map1.Initialize(0, 0);
	down1.Initialize(80, 410);

	map2.Initialize(0, 0);
	up2.Initialize(325, 300);
	down2.Initialize(330, 440);

	map3.Initialize(0, 0);
	down3.Initialize(350, 430);
	left3.Initialize(255, 290);
	right3.Initialize(350, 190);

	map4.Initialize(0, 0);
	left4.Initialize(25, 400);

	map5.Initialize(0, 0);
	up5.Initialize(325, 300);
	down5.Initialize(330, 440);

	map6.Initialize(0, 0);
	up6.Initialize(325, 375);
	down6.Initialize(330, 450);
	left6.Initialize(80, 410);
	right6.Initialize(575, 410);
	message6.Initialize(50, 220);
	shard6.Initialize(540, 340);
	clock6.Initialize(300, 60);

	map7.Initialize(0, 0);
	down7.Initialize(145, 150);
	left7.Initialize(25, 400);
	lcdTV7.Initialize(50, 220);
	safeBox7.Initialize(230, 340);
	clue7.Initialize(380, 340);
	tap7.Initialize(560, 400);
	shard7.Initialize(480, 400);
	draw7.Initialize(25, 120);

	map8.Initialize(0, 0);
	down8.Initialize(320, 440);
	left8.Initialize(125, 350);
	box8.Initialize(480, 320);
	message8.Initialize(10, 250);
	gpu8.Initialize(25, 400);

	map9.Initialize(0, 0);
	down9.Initialize(225, 425);
	cpu9.Initialize(75, 300);
	code9.Initialize(405, 130);
	safeBox9.Initialize(405, 290);

	map10.Initialize(0, 0);
	left10.Initialize(25, 400);
	message10.Initialize(420, 300);
	memory10.Initialize(320, 390);
	shard10.Initialize(120, 390);

	map11.Initialize(0, 0);
	down11.Initialize(315, 450);
	left11.Initialize(65, 410);
	right11.Initialize(575, 410);

	map12.Initialize(0, 0);
	left12.Initialize(80, 410);

	black1.Initialize(0, 0);
	ax.Initialize(0, 0);
	cpu.Initialize(80, 0);
	gpu.Initialize(160, 0);
	goldKey.Initialize(240, 0);
	silverKey.Initialize(320, 0);
	copperKey.Initialize(400, 0);
	memory.Initialize(480, 0);
	shard.Initialize(0, 80);
	slad.Initialize(80, 80);
	tap.Initialize(160, 80);
	shardBag7.Initialize(240, 80);
	draw.Initialize(320, 80);
	shardBag10.Initialize(400, 80);

	bag.Initialize(560, 0);
	clickToStart.Initialize(0, 0);
}

void CGameStateRun::MoveMap()
{
	map0.OnMove();

	map1.OnMove();
	down1.OnMove();

	map2.OnMove();
	up2.OnMove();
	down2.OnMove();

	map3.OnMove();
	down3.OnMove();
	left3.OnMove();
	right3.OnMove();

	map4.OnMove();
	left4.OnMove();

	map5.OnMove();
	up5.OnMove();
	down5.OnMove();

	map6.OnMove();
	up6.OnMove();
	down6.OnMove();
	left6.OnMove();
	right6.OnMove();
	message6.OnMove();
	shard6.OnMove();
	clock6.OnMove();

	map7.OnMove();
	down7.OnMove();
	left7.OnMove();
	lcdTV7.OnMove();
	safeBox7.OnMove();
	clue7.OnMove();
	tap7.OnMove();
	shard7.OnMove();
	draw7.OnMove();

	map8.OnMove();
	down8.OnMove();
	left8.OnMove();
	box8.OnMove();
	message8.OnMove();
	gpu8.OnMove();
	safeBox9.OnMove();

	map9.OnMove();
	down9.OnMove();
	cpu9.OnMove();
	code9.OnMove();

	map10.OnMove();
	left10.OnMove();
	message10.OnMove();
	memory10.OnMove();
	shard10.OnMove();

	map11.OnMove();
	down11.OnMove();
	left11.OnMove();
	right11.OnMove();

	map12.OnMove();
	left12.OnMove();

	black1.OnMove();
	ax.OnMove();
	cpu.OnMove();
	gpu.OnMove();
	goldKey.OnMove();
	silverKey.OnMove();
	copperKey.OnMove();
	memory.OnMove();
	shard.OnMove();
	slad.OnMove();
	tap.OnMove();
	shardBag7.OnMove();
	draw.OnMove();
	shardBag10.OnMove();

	bag.OnMove();
	clickToStart.OnMove();
}

void CGameStateRun::LoadMap()
{
	map0.LoadBitmap(IDB_MAP0);

	map1.LoadBitmap(IDB_MAP1);
	down1.LoadBitmap(IDB_DOWN1);

	map2.LoadBitmap(IDB_MAP2);
	up2.LoadBitmap(IDB_UP2);
	down2.LoadBitmap(IDB_DOWN2);

	map3.LoadBitmap(IDB_MAP3);
	down3.LoadBitmap(IDB_DOWN3);
	left3.LoadBitmap(IDB_LEFT3);
	right3.LoadBitmap(IDB_RIGHT3);

	map4.LoadBitmap(IDB_MAP4);
	left4.LoadBitmap(IDB_LEFT4);

	map5.LoadBitmap(IDB_MAP5);
	up5.LoadBitmap(IDB_UP5);
	down5.LoadBitmap(IDB_DOWN5);

	map6.LoadBitmap(IDB_MAP6);
	up6.LoadBitmap(IDB_UP6);
	down6.LoadBitmap(IDB_DOWN6);
	left6.LoadBitmap(IDB_LEFT6);
	right6.LoadBitmap(IDB_RIGHT6);
	message6.LoadBitmap(IDB_MESSAGE6);
	shard6.LoadBitmap(IDB_SHARD6);
	clock6.LoadBitmap(IDB_CLOCK6);

	map7.LoadBitmap(IDB_MAP7);
	down7.LoadBitmap(IDB_DOWN7);
	left7.LoadBitmap(IDB_LEFT7);
	lcdTV7.LoadBitmap(IDB_LCDTV7);
	safeBox7.LoadBitmap(IDB_SAFEBOX7);
	clue7.LoadBitmap(IDB_CLUE7);
	tap7.LoadBitmap(IDB_TAP7);
	shard7.LoadBitmap(IDB_SHARD7);
	draw7.LoadBitmap(IDB_DRAW7);

	map8.LoadBitmap(IDB_MAP8);
	down8.LoadBitmap(IDB_DOWN8);
	left8.LoadBitmap(IDB_LEFT8);
	box8.LoadBitmap(IDB_BOX8);
	message8.LoadBitmap(IDB_MESSAGE8);
	gpu8.LoadBitmap(IDB_GPU8);

	map9.LoadBitmap(IDB_MAP9);
	down9.LoadBitmap(IDB_DOWN9);
	cpu9.LoadBitmap(IDB_CPU9);
	code9.LoadBitmap(IDB_CODE9);
	safeBox9.LoadBitmap(IDB_SAFEBOX9);

	map10.LoadBitmap(IDB_MAP10);
	left10.LoadBitmap(IDB_LEFT10);
	message10.LoadBitmap(IDB_MESSAGE10);
	memory10.LoadBitmap(IDB_MEMORY10);
	shard10.LoadBitmap(IDB_SHARD10);

	map11.LoadBitmap(IDB_MAP11);
	down11.LoadBitmap(IDB_DOWN11);
	left11.LoadBitmap(IDB_LEFT11);
	right11.LoadBitmap(IDB_RIGHT11);

	map12.LoadBitmap(IDB_MAP12);
	left12.LoadBitmap(IDB_LEFT12);

	black1.LoadBitmap(IDB_BLACK1);
	ax.LoadBitmap(IDB_AX);
	cpu.LoadBitmap(IDB_CPU);
	gpu.LoadBitmap(IDB_GPU);
	goldKey.LoadBitmap(IDB_GOLDKEY);
	silverKey.LoadBitmap(IDB_SILVERKEY);
	copperKey.LoadBitmap(IDB_COPPERKEY);
	memory.LoadBitmap(IDB_MEMORY);
	shard.LoadBitmap(IDB_SHARD);
	slad.LoadBitmap(IDB_SLAD);
	tap.LoadBitmap(IDB_TAP);
	shardBag7.LoadBitmap(IDB_SHARDBAG7);
	draw.LoadBitmap(IDB_DRAW);
	shardBag10.LoadBitmap(IDB_SHARDBAG10);

	bag.LoadBitmap(IDB_BAG);
	clickToStart.LoadBitmap(IDB_CLICKTOSTART);
}

void CGameStateRun::ShowMap()
{
	map0.OnShow();

	map1.OnShow();
	down1.OnShow();

	map2.OnShow();
	up2.OnShow();
	down2.OnShow();

	map3.OnShow();
	down3.OnShow();
	left3.OnShow();
	right3.OnShow();

	map4.OnShow();
	left4.OnShow();

	map5.OnShow();
	up5.OnShow();
	down5.OnShow();

	map6.OnShow();
	up6.OnShow();
	down6.OnShow();
	left6.OnShow();
	right6.OnShow();
	message6.OnShow();
	shard6.OnShow();
	clock6.OnShow();

	map7.OnShow();
	down7.OnShow();
	left7.OnShow();
	lcdTV7.OnShow();
	safeBox7.OnShow();
	clue7.OnShow();
	tap7.OnShow();
	shard7.OnShow();
	draw7.OnShow();

	map8.OnShow();
	down8.OnShow();
	left8.OnShow();
	box8.OnShow();
	message8.OnShow();
	gpu8.OnShow();

	map9.OnShow();
	down9.OnShow();
	cpu9.OnShow();
	code9.OnShow();
	safeBox9.OnShow();

	map10.OnShow();
	left10.OnShow();
	message10.OnShow();
	memory10.OnShow();
	shard10.OnShow();

	map11.OnShow();
	down11.OnShow();
	left11.OnShow();
	right11.OnShow();


	map12.OnShow();
	left12.OnShow();

	black1.OnShow();
	ax.OnShow();
	cpu.OnShow();
	gpu.OnShow();
	goldKey.OnShow();
	silverKey.OnShow();
	copperKey.OnShow();
	memory.OnShow();
	shard.OnShow();
	slad.OnShow();
	tap.OnShow();
	shardBag7.OnShow();
	draw.OnShow();
	shardBag10.OnShow();

	bag.OnShow();
	clickToStart.OnShow();
}

void CGameStateRun::MVMap1Left()
{
	map1.SetMovingLeft(true);
	down1.SetMovingLeft(true);
}

void CGameStateRun::MVMap1Right()
{
	map1.SetMovingRight(true);
	down1.SetMovingRight(true);
}

void CGameStateRun::MVMap2Left()
{
	map2.SetMovingLeft(true);
	up2.SetMovingLeft(true);
	down2.SetMovingLeft(true);
}

void CGameStateRun::MVMap2Right()
{
	map2.SetMovingRight(true);
	up2.SetMovingRight(true);
	down2.SetMovingRight(true);
}

void CGameStateRun::MVMap3Left()
{
	map3.SetMovingLeft(true);
	down3.SetMovingLeft(true);
	left3.SetMovingLeft(true);
	right3.SetMovingLeft(true);
}

void CGameStateRun::MVMap3Right()
{
	map3.SetMovingRight(true);
	down3.SetMovingRight(true);
	left3.SetMovingRight(true);
	right3.SetMovingRight(true);
}

void CGameStateRun::MVMap4Left()
{
	map4.SetMovingLeft(true);
	left4.SetMovingLeft(true);
}

void CGameStateRun::MVMap4Right()
{
	map4.SetMovingRight(true);
	left4.SetMovingRight(true);
}

void CGameStateRun::MVMap5Left()
{
	map5.SetMovingLeft(true);
	up5.SetMovingLeft(true);
	down5.SetMovingLeft(true);
}

void CGameStateRun::MVMap5Right()
{
	map5.SetMovingRight(true);
	up5.SetMovingRight(true);
	down5.SetMovingRight(true);
}

void CGameStateRun::MVMap6Left()
{
	map6.SetMovingLeft(true);
	up6.SetMovingLeft(true);
	down6.SetMovingLeft(true);
	left6.SetMovingLeft(true);
	right6.SetMovingLeft(true);
	message6.SetMovingLeft(true);
	if (bagArray[8] == 0)
	    shard6.SetMovingLeft(true);
	clock6.SetMovingLeft(true);
}

void CGameStateRun::MVMap6Right()
{
	map6.SetMovingRight(true);
	up6.SetMovingRight(true);
	down6.SetMovingRight(true);
	left6.SetMovingRight(true);
	right6.SetMovingRight(true);
	message6.SetMovingRight(true);
	shard6.SetMovingRight(true);
	clock6.SetMovingRight(true);
}

void CGameStateRun::MVMap7Left()
{
	map7.SetMovingLeft(true);
	down7.SetMovingLeft(true);
	left7.SetMovingLeft(true);
	lcdTV7.SetMovingLeft(true);
	safeBox7.SetMovingLeft(true);
	clue7.SetMovingLeft(true);
	if (bagArray[10] == 0)
		tap7.SetMovingLeft(true);
	if (bagArray[11] == 0)
		shard7.SetMovingLeft(true);
	if (bagArray[12] == 0)
		draw7.SetMovingLeft(true);
}

void CGameStateRun::MVMap7Right()
{
	map7.SetMovingRight(true);
	down7.SetMovingRight(true);
	left7.SetMovingRight(true);
	lcdTV7.SetMovingRight(true);
	safeBox7.SetMovingRight(true);
	clue7.SetMovingRight(true);
	tap7.SetMovingRight(true);
	shard7.SetMovingRight(true);
	draw7.SetMovingRight(true);
}

void CGameStateRun::MVMap8Left()
{
	map8.SetMovingLeft(true);
	down8.SetMovingLeft(true);
	left8.SetMovingLeft(true);
	box8.SetMovingLeft(true);
	message8.SetMovingLeft(true);
	if (bagArray[2] == 0)
		gpu8.SetMovingLeft(true);
}

void CGameStateRun::MVMap8Right()
{
	map8.SetMovingRight(true);
	down8.SetMovingRight(true);
	left8.SetMovingRight(true);
	box8.SetMovingRight(true);
	message8.SetMovingRight(true);
	gpu8.SetMovingRight(true);
}

void CGameStateRun::MVMap9Left()
{
	map9.SetMovingLeft(true);
	down9.SetMovingLeft(true);
	if (bagArray[1] == 0)
		cpu9.SetMovingLeft(true);
	code9.SetMovingLeft(true);
	safeBox9.SetMovingLeft(true);
}

void CGameStateRun::MVMap9Right()
{
	map9.SetMovingRight(true);
	down9.SetMovingRight(true);
	cpu9.SetMovingRight(true);
	code9.SetMovingRight(true);
	safeBox9.SetMovingRight(true);
}

void CGameStateRun::MVMap10Left()
{
	map10.SetMovingLeft(true);
	left10.SetMovingLeft(true);
	message10.SetMovingLeft(true);
	if (bagArray[6] == 0)
		memory10.SetMovingLeft(true);
	if (bagArray[13] == 0)
		shard10.SetMovingLeft(true);
}

void CGameStateRun::MVMap10Right()
{
	map10.SetMovingRight(true);
	left10.SetMovingRight(true);
	message10.SetMovingRight(true);
	memory10.SetMovingRight(true);
	shard10.SetMovingRight(true);
}

void CGameStateRun::MVMap11Left()
{
	map11.SetMovingLeft(true);
	down11.SetMovingLeft(true);
	left11.SetMovingLeft(true);
	right11.SetMovingLeft(true);
}

void CGameStateRun::MVMap11Right()
{
	map11.SetMovingRight(true);
	down11.SetMovingRight(true);
	left11.SetMovingRight(true);
	right11.SetMovingRight(true);
}

void CGameStateRun::MVMap12Left()
{
	map12.SetMovingLeft(true);
	left12.SetMovingLeft(true);
}

void CGameStateRun::MVMap12Right()
{
	map12.SetMovingRight(true);
	left12.SetMovingRight(true);
}

void CGameStateRun::MVClickToStartRight()
{
	clickToStart.SetMovingRight(true);
}

void CGameStateRun::MVBlack1Left()
{
	black1.SetMovingLeft(true);
	if (bagArray[0] == 1)
	    ax.SetMovingLeft(true);
	if (bagArray[1] == 1)
	    cpu.SetMovingLeft(true);
	if (bagArray[2] == 1)
	    gpu.SetMovingLeft(true);
	if (bagArray[3] == 1)
	    goldKey.SetMovingLeft(true);
	if (bagArray[4] == 1)
	    silverKey.SetMovingLeft(true);
	if (bagArray[5] == 1)
	    copperKey.SetMovingLeft(true);
	if (bagArray[6] == 1)
	    memory.SetMovingLeft(true);
	if (bagArray[8] == 1)
	    shard.SetMovingLeft(true);
	if (bagArray[9] == 1)
	    slad.SetMovingLeft(true);
	if (bagArray[10] == 1)
	    tap.SetMovingLeft(true);
	if (bagArray[11] == 1)
		shardBag7.SetMovingLeft(true);
	if (bagArray[12] == 1)
		draw.SetMovingLeft(true);
	if (bagArray[13] == 1)
		shardBag10.SetMovingLeft(true);
}

void CGameStateRun::MVBlack1Right()
{
	black1.SetMovingRight(true);
	if (bagArray[0] == 1)
		ax.SetMovingRight(true);
	if (bagArray[1] == 1)
		cpu.SetMovingRight(true);
	if (bagArray[2] == 1)
		gpu.SetMovingRight(true);
	if (bagArray[3] == 1)
		goldKey.SetMovingRight(true);
	if (bagArray[4] == 1)
		silverKey.SetMovingRight(true);
	if (bagArray[5] == 1)
		copperKey.SetMovingRight(true);
	if (bagArray[6] == 1)
		memory.SetMovingRight(true);
	if (bagArray[8] == 1)
		shard.SetMovingRight(true);
	if (bagArray[9] == 1)
		slad.SetMovingRight(true);
	if (bagArray[10] == 1)
		tap.SetMovingRight(true);
	if (bagArray[11] == 1)
		shardBag7.SetMovingRight(true);
	if (bagArray[12] == 1)
		draw.SetMovingRight(true);
	if (bagArray[13] == 1)
		shardBag10.SetMovingRight(true);
}

void CGameStateRun::OnBeginState()
{
	const int BALL_GAP = 90;
	const int BALL_XY_OFFSET = 45;
	const int BALL_PER_ROW = 7;
	const int HITS_LEFT = 10;
	const int HITS_LEFT_X = 590;
	const int HITS_LEFT_Y = 0;
	const int BACKGROUND_X = 60;
	const int ANIMATION_SPEED = 15;
	for (int i = 0; i < NUMBALLS; i++) {				// 設定球的起始座標
		int x_pos = i % BALL_PER_ROW;
		int y_pos = i / BALL_PER_ROW;
		ball[i].SetXY(x_pos * BALL_GAP + BALL_XY_OFFSET, y_pos * BALL_GAP + BALL_XY_OFFSET);
		ball[i].SetDelay(x_pos);
		ball[i].SetIsAlive(true);
	}
	eraser.Initialize();
	InitMap();
	background.SetTopLeft(BACKGROUND_X,0);				// 設定背景的起始座標
	help.SetTopLeft(0, SIZE_Y - help.Height());			// 設定說明圖的起始座標
	hits_left.SetInteger(HITS_LEFT);					// 指定剩下的撞擊數
	hits_left.SetTopLeft(HITS_LEFT_X,HITS_LEFT_Y);		// 指定剩下撞擊數的座標
	CAudio::Instance()->Play(AUDIO_LAKE, true);			// 撥放 WAVE
	CAudio::Instance()->Play(AUDIO_DING, false);		// 撥放 WAVE
	CAudio::Instance()->Play(AUDIO_NTUT, true);			// 撥放 MIDI
}

void CGameStateRun::OnMove()							// 移動遊戲元素
{
	//
	// 如果希望修改cursor的樣式，則將下面程式的commment取消即可
	//
	// SetCursor(AfxGetApp()->LoadCursor(IDC_GAMECURSOR));
	//
	// 移動背景圖的座標
	//bitmap3.SetTopLeft(10, 10);
	
	if (picX <= SIZE_Y) {
		picX += 5;
		picY += 5;
	}
	else {
		picX = picY = 0;
	}
	bitmap3.SetTopLeft(picX, picY);
	//
	if (background.Top() > SIZE_Y)
		background.SetTopLeft(60 ,-background.Height());
	background.SetTopLeft(background.Left(),background.Top()+1);
	//
	// 移動球
	
	c_practice.OnMove();
	//
	int i;
	for (i=0; i < NUMBALLS; i++)
		ball[i].OnMove();
	//
	// 移動擦子
	//
	eraser.OnMove();
	MoveMap();
	//
	// 判斷擦子是否碰到球
	//
	for (i=0; i < NUMBALLS; i++)
		if (ball[i].IsAlive() && ball[i].HitEraser(&eraser)) {
			ball[i].SetIsAlive(false);
			CAudio::Instance()->Play(AUDIO_DING);
			hits_left.Add(-1);
			//
			// 若剩餘碰撞次數為0，則跳到Game Over狀態
			//
			if (hits_left.GetInteger() <= 0) {
				CAudio::Instance()->Stop(AUDIO_LAKE);	// 停止 WAVE
				CAudio::Instance()->Stop(AUDIO_NTUT);	// 停止 MIDI
				GotoGameState(GAME_STATE_OVER);
			}
		}
	//
	// 移動彈跳的球
	//
	gamemap.OnMove();
	bball.OnMove();
}

void CGameStateRun::OnInit()  								// 遊戲的初值及圖形設定
{
	//
	// 當圖很多時，OnInit載入所有的圖要花很多時間。為避免玩遊戲的人
	//     等的不耐煩，遊戲會出現「Loading ...」，顯示Loading的進度。
	//
	
	ShowInitProgress(33);	// 接個前一個狀態的進度，此處進度視為33%
	//
	// 開始載入資料
	bitmap3.LoadBitmap(IDB_BITMAP3);
	c_practice.LoadBitmap();
	gamemap.LoadBitmap();
	//
	int i;
	for (i = 0; i < NUMBALLS; i++)	
		ball[i].LoadBitmap();								// 載入第i個球的圖形
	eraser.LoadBitmap();
	LoadMap();
	background.LoadBitmap(IDB_BACKGROUND);					// 載入背景的圖形
	//
	// 完成部分Loading動作，提高進度
	//
	ShowInitProgress(50);
	Sleep(300); // 放慢，以便看清楚進度，實際遊戲請刪除此Sleep
	//
	// 繼續載入其他資料
	//
	help.LoadBitmap(IDB_HELP,RGB(255,255,255));				// 載入說明的圖形
	corner.LoadBitmap(IDB_CORNER);							// 載入角落圖形
	corner.ShowBitmap(background);							// 將corner貼到background
	bball.LoadBitmap();										// 載入圖形
	hits_left.LoadBitmap();									
	CAudio::Instance()->Load(AUDIO_DING,  "sounds\\ding.wav");	// 載入編號0的聲音ding.wav
	CAudio::Instance()->Load(AUDIO_LAKE,  "sounds\\lake.mp3");	// 載入編號1的聲音lake.mp3
	CAudio::Instance()->Load(AUDIO_NTUT,  "sounds\\ntut.mid");	// 載入編號2的聲音ntut.mid
	//
	// 此OnInit動作會接到CGameStaterOver::OnInit()，所以進度還沒到100%
	//
}

void CGameStateRun::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	
	const char KEY_LEFT  = 0x25; // keyboard左箭頭
	const char KEY_UP    = 0x26; // keyboard上箭頭
	const char KEY_RIGHT = 0x27; // keyboard右箭頭
	const char KEY_DOWN  = 0x28; // keyboard下箭頭
	gamemap.OnKeyDown(nChar);
	if (nChar == KEY_LEFT)
		eraser.SetMovingLeft(true);
	if (nChar == KEY_RIGHT)
		eraser.SetMovingRight(true);
	if (nChar == KEY_UP)
		eraser.SetMovingUp(true);
	if (nChar == KEY_DOWN)
		eraser.SetMovingDown(true);
}

void CGameStateRun::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const char KEY_LEFT  = 0x25; // keyboard左箭頭
	const char KEY_UP    = 0x26; // keyboard上箭頭
	const char KEY_RIGHT = 0x27; // keyboard右箭頭
	const char KEY_DOWN  = 0x28; // keyboard下箭頭
	if (nChar == KEY_LEFT)
		eraser.SetMovingLeft(false);
	if (nChar == KEY_RIGHT)
		eraser.SetMovingRight(false);
	if (nChar == KEY_UP)
		eraser.SetMovingUp(false);
	if (nChar == KEY_DOWN)
		eraser.SetMovingDown(false);
}

void CGameStateRun::OnLButtonDown(UINT nFlags, CPoint point)  // 處理滑鼠的動作
{
//	eraser.SetMovingLeft(true);
	if (mapNow == 0)
	{
		MVMap2Right();
		MVMap3Right();
		MVMap4Right();
		MVMap5Right();
		MVMap6Right();
		MVMap7Right();
		MVMap8Right();
		MVMap9Right();
		MVMap10Right();
		MVMap11Right();
		MVMap12Right();
		MVBlack1Right();
		MVClickToStartRight();
		for (int i = 0; i < 48; i++)
			bagArray[i] = 0;
		mapNow = 1;
	}
	else if (point.x >= bag.GetX1() && point.x <= bag.GetX2() && point.y >= bag.GetY1() && point.y <= bag.GetY2())
	{
		if (mapTemp == -1)
			MVBlack1Left();
		else
			MVBlack1Right();

		int temp = mapNow;
		mapNow = mapTemp;
		mapTemp = temp;
	}
	else if (mapNow == -1)
	{
		int count = -1;
		int objTemp = objNow;
		for (int i = 0; i < 480; i += 80)
		{
			if (objNow != objTemp)
				break;
			for (int j = 0; j < 640; j += 80)
			{
				count++;
				if (point.x >= j && point.x <= j + 80 && point.y >= i && point.y <= i + 80 && bagArray[count] == 1)
				{
					int temp = mapNow;
					mapNow = mapTemp;
					mapTemp = temp;
					objNow = count;
					MVBlack1Right();
					break;
				}
			}
		}
	}
	else if (mapNow == 1)
	{
		if (point.x >= down1.GetX1() && point.x <= down1.GetX2() && point.y >= down1.GetY1() && point.y <= down1.GetY2())
		{
			MVMap2Left();
			MVMap1Right();
			mapNow = 2;
		}
	}
	else if (mapNow == 2)
	{
		if (point.x >= up2.GetX1() && point.x <= up2.GetX2() && point.y >= up2.GetY1() && point.y <= up2.GetY2())
		{
			MVMap3Left();
			MVMap2Right();
			mapNow = 3;
		}
		else if (point.x >= down2.GetX1() && point.x <= down2.GetX2() && point.y >= down2.GetY1() && point.y <= down2.GetY2())
		{
			MVMap1Left();
			MVMap2Right();
			mapNow = 1;
		}
	}
	else if (mapNow == 3)
	{
		if (point.x >= down3.GetX1() && point.x <= down3.GetX2() && point.y >= down3.GetY1() && point.y <= down3.GetY2())
		{
			MVMap2Left();
			MVMap3Right();
			mapNow = 2;
		}
		else if (point.x >= left3.GetX1() && point.x <= left3.GetX2() && point.y >= left3.GetY1() && point.y <= left3.GetY2())
		{
			MVMap4Left();
			MVMap3Right();
			mapNow = 4;
		}
		else if (point.x >= right3.GetX1() && point.x <= right3.GetX2() && point.y >= right3.GetY1() && point.y <= right3.GetY2())
		{
			MVMap5Left();
			MVMap3Right();
			mapNow = 5;
		}
	}
	else if (mapNow == 4)
	{
		if (point.x >= left4.GetX1() && point.x <= left4.GetX2() && point.y >= left4.GetY1() && point.y <= left4.GetY2())
		{
			MVMap3Left();
			MVMap4Right();
			mapNow = 3;
		}
	}
	else if (mapNow == 5)
	{
		if (point.x >= up5.GetX1() && point.x <= up5.GetX2() && point.y >= up5.GetY1() && point.y <= up5.GetY2())
		{
			MVMap6Left();
			MVMap5Right();
			mapNow = 6;
		}
		else if (point.x >= down5.GetX1() && point.x <= down5.GetX2() && point.y >= down5.GetY1() && point.y <= down5.GetY2())
		{
			MVMap3Left();
			MVMap5Right();
			mapNow = 3;
		}
	}
	else if (mapNow == 6)
	{
		if (point.x >= up6.GetX1() && point.x <= up6.GetX2() && point.y >= up6.GetY1() && point.y <= up6.GetY2())
		{
			//if (objNow == 3)
			//{
				MVMap10Left();
				MVMap6Right();
				mapNow = 10;
			//}
		}
		else if (point.x >= down6.GetX1() && point.x <= down6.GetX2() && point.y >= down6.GetY1() && point.y <= down6.GetY2())
		{
			MVMap5Left();
			MVMap6Right();
			mapNow = 5;
		}
		else if (point.x >= left6.GetX1() && point.x <= left6.GetX2() && point.y >= left6.GetY1() && point.y <= left6.GetY2())
		{
			MVMap7Left();
			MVMap6Right();
			mapNow = 7;
		}
		else if (point.x >= right6.GetX1() && point.x <= right6.GetX2() && point.y >= right6.GetY1() && point.y <= right6.GetY2())
		{
			//if (objNow == 5)
			//{
				MVMap11Left();
				MVMap6Right();
				mapNow = 11;
			//}
		}
		else if (point.x >= shard6.GetX1() && point.x <= shard6.GetX2() && point.y >= shard6.GetY1() && point.y <= shard6.GetY2())
		{
			shard6.SetMovingRight(true);
			bagArray[8] = 1;
		}
	}
	else if (mapNow == 7)
	{
		if (point.x >= down7.GetX1() && point.x <= down7.GetX2() && point.y >= down7.GetY1() && point.y <= down7.GetY2())
		{
			MVMap8Left();
			MVMap7Right();
			mapNow = 8;
		}
		else if (point.x >= left7.GetX1() && point.x <= left7.GetX2() && point.y >= left7.GetY1() && point.y <= left7.GetY2())
		{
			MVMap6Left();
			MVMap7Right();
			mapNow = 6;
		}
		else if (point.x >= tap7.GetX1() && point.x <= tap7.GetX2() && point.y >= tap7.GetY1() && point.y <= tap7.GetY2())
		{
			tap7.SetMovingRight(true);
			bagArray[10] = 1;
		}
		else if (point.x >= shard7.GetX1() && point.x <= shard7.GetX2() && point.y >= shard7.GetY1() && point.y <= shard7.GetY2())
		{
			shard7.SetMovingRight(true);
			bagArray[11] = 1;
		}
		else if (point.x >= draw7.GetX1() && point.x <= draw7.GetX2() && point.y >= draw7.GetY1() && point.y <= draw7.GetY2())
		{
			draw7.SetMovingRight(true);
			bagArray[12] = 1;
		}
	}
	else if (mapNow == 8)
	{
		if (point.x >= down8.GetX1() && point.x <= down8.GetX2() && point.y >= down8.GetY1() && point.y <= down8.GetY2())
		{
			MVMap7Left();
			MVMap8Right();
			mapNow = 7;
		}
		else if (point.x >= left8.GetX1() && point.x <= left8.GetX2() && point.y >= left8.GetY1() && point.y <= left8.GetY2())
		{
			//if (objNow == 4)
			//{
				MVMap9Left();
				MVMap8Right();
				mapNow = 9;
			//}
		}
		else if (point.x >= gpu8.GetX1() && point.x <= gpu8.GetX2() && point.y >= gpu8.GetY1() && point.y <= gpu8.GetY2())
		{
			gpu8.SetMovingRight(true);
			bagArray[2] = 1;
		}
	}
	else if (mapNow == 9)
	{
		if (point.x >= down9.GetX1() && point.x <= down9.GetX2() && point.y >= down9.GetY1() && point.y <= down9.GetY2())
		{
			MVMap8Left();
			MVMap9Right();
			mapNow = 8;
		}
		else if (point.x >= cpu9.GetX1() && point.x <= cpu9.GetX2() && point.y >= cpu9.GetY1() && point.y <= cpu9.GetY2())
		{
			cpu9.SetMovingRight(true);
			bagArray[1] = 1;
		}
	}
	else if (mapNow == 10)
	{
		if (point.x >= left10.GetX1() && point.x <= left10.GetX2() && point.y >= left10.GetY1() && point.y <= left10.GetY2())
		{
			MVMap6Left();
			MVMap10Right();
			mapNow = 6;
		}
		else if (point.x >= memory10.GetX1() && point.x <= memory10.GetX2() && point.y >= memory10.GetY1() && point.y <= memory10.GetY2())
		{
			memory10.SetMovingRight(true);
			bagArray[6] = 1;
		}
		else if (point.x >= shard10.GetX1() && point.x <= shard10.GetX2() && point.y >= shard10.GetY1() && point.y <= shard10.GetY2())
		{
			shard10.SetMovingRight(true);
			bagArray[13] = 1;
		}
	}
	else if (mapNow == 11)
	{
		if (point.x >= down11.GetX1() && point.x <= down11.GetX2() && point.y >= down11.GetY1() && point.y <= down11.GetY2())
		{
			MVMap6Left();
			MVMap11Right();
			mapNow = 6;
		}
		else if (point.x >= right11.GetX1() && point.x <= right11.GetX2() && point.y >= right11.GetY1() && point.y <= right11.GetY2())
		{
			MVMap12Left();
			MVMap11Right();
			mapNow = 12;
		}
	}
	else if (mapNow == 12)
	{
		if (point.x >= left12.GetX1() && point.x <= left12.GetX2() && point.y >= left12.GetY1() && point.y <= left12.GetY2())
		{
			MVMap11Left();
			MVMap12Right();
			mapNow = 11;
		}
	}
}

void CGameStateRun::OnLButtonUp(UINT nFlags, CPoint point)	// 處理滑鼠的動作
{
//	eraser.SetMovingLeft(false);
	map0.SetMovingLeft(false);
    map1.SetMovingLeft(false);
	down1.SetMovingLeft(false);
    map2.SetMovingLeft(false);
	up2.SetMovingLeft(false);
	down2.SetMovingLeft(false);
    map3.SetMovingLeft(false);
	down3.SetMovingLeft(false);
	left3.SetMovingLeft(false);
	right3.SetMovingLeft(false);
    map4.SetMovingLeft(false);
	left4.SetMovingLeft(false);
    map5.SetMovingLeft(false);
	up5.SetMovingLeft(false);
	down5.SetMovingLeft(false);
    map6.SetMovingLeft(false);
	up6.SetMovingLeft(false);
	down6.SetMovingLeft(false);
	left6.SetMovingLeft(false);
	right6.SetMovingLeft(false);
	message6.SetMovingLeft(false);
	shard6.SetMovingLeft(false);
	clock6.SetMovingLeft(false);
	map7.SetMovingLeft(false);
	down7.SetMovingLeft(false);
	left7.SetMovingLeft(false);
	lcdTV7.SetMovingLeft(false);
	safeBox7.SetMovingLeft(false);
	clue7.SetMovingLeft(false);
	tap7.SetMovingLeft(false);
	shard7.SetMovingLeft(false);
	draw7.SetMovingLeft(false);
    map8.SetMovingLeft(false);
	down8.SetMovingLeft(false);
	left8.SetMovingLeft(false);
	box8.SetMovingLeft(false);
	message8.SetMovingLeft(false);
	gpu8.SetMovingLeft(false);
	map9.SetMovingLeft(false);
	down9.SetMovingLeft(false);
	cpu9.SetMovingLeft(false);
	code9.SetMovingLeft(false);
	safeBox9.SetMovingLeft(false);
	map10.SetMovingLeft(false);
	left10.SetMovingLeft(false);
	message10.SetMovingLeft(false);
	memory10.SetMovingLeft(false);
	shard10.SetMovingLeft(false);
	map11.SetMovingLeft(false);
	down11.SetMovingLeft(false);
	left11.SetMovingLeft(false);
	right11.SetMovingLeft(false);
	map12.SetMovingLeft(false);
	left12.SetMovingLeft(false);
	black1.SetMovingLeft(false);
	ax.SetMovingLeft(false);
	cpu.SetMovingLeft(false);
	gpu.SetMovingLeft(false);
	goldKey.SetMovingLeft(false);
	silverKey.SetMovingLeft(false);
	copperKey.SetMovingLeft(false);
	memory.SetMovingLeft(false);
	shard.SetMovingLeft(false);
	slad.SetMovingLeft(false);
	tap.SetMovingLeft(false);
	shardBag7.SetMovingLeft(false);
	draw.SetMovingLeft(false);
	shardBag10.SetMovingLeft(false);
	clickToStart.SetMovingLeft(false);

	map0.SetMovingRight(false);
	map1.SetMovingRight(false);
	down1.SetMovingRight(false);
	map2.SetMovingRight(false);
	up2.SetMovingRight(false);
	down2.SetMovingRight(false);
	map3.SetMovingRight(false);
	down3.SetMovingRight(false);
	left3.SetMovingRight(false);
	right3.SetMovingRight(false);
	map4.SetMovingRight(false);
	left4.SetMovingRight(false);
	map5.SetMovingRight(false);
	up5.SetMovingRight(false);
	down5.SetMovingRight(false);
	map6.SetMovingRight(false);
	up6.SetMovingRight(false);
	down6.SetMovingRight(false);
	left6.SetMovingRight(false);
	right6.SetMovingRight(false);
	message6.SetMovingRight(false);
	shard6.SetMovingRight(false);
	clock6.SetMovingRight(false);
	map7.SetMovingRight(false);
	down7.SetMovingRight(false);
	left7.SetMovingRight(false);
	lcdTV7.SetMovingRight(false);
	safeBox7.SetMovingRight(false);
	clue7.SetMovingRight(false);
	tap7.SetMovingRight(false);
	shard7.SetMovingRight(false);
	draw7.SetMovingRight(false);
	map8.SetMovingRight(false);
	down8.SetMovingRight(false);
	left8.SetMovingRight(false);
	box8.SetMovingRight(false);
	message8.SetMovingRight(false);
	gpu8.SetMovingRight(false);
	map9.SetMovingRight(false);
	down9.SetMovingRight(false);
	cpu9.SetMovingRight(false);
	code9.SetMovingRight(false);
	safeBox9.SetMovingRight(false);
	map10.SetMovingRight(false);
	left10.SetMovingRight(false);
	message10.SetMovingRight(false);
	memory10.SetMovingRight(false);
	shard10.SetMovingRight(false);
	map11.SetMovingRight(false);
	down11.SetMovingRight(false);
	left11.SetMovingRight(false);
	right11.SetMovingRight(false);
	map12.SetMovingRight(false);
	left12.SetMovingRight(false);
	black1.SetMovingRight(false);
	ax.SetMovingRight(false);
	cpu.SetMovingRight(false);
	gpu.SetMovingRight(false);
	goldKey.SetMovingRight(false);
	silverKey.SetMovingRight(false);
	copperKey.SetMovingRight(false);
	memory.SetMovingRight(false);
	shard.SetMovingRight(false);
	slad.SetMovingRight(false);
	tap.SetMovingRight(false);
	shardBag7.SetMovingRight(false);
	draw.SetMovingRight(false);
	shardBag10.SetMovingRight(false);
	clickToStart.SetMovingRight(false);
}

void CGameStateRun::OnMouseMove(UINT nFlags, CPoint point)	// 處理滑鼠的動作
{
	// 沒事。如果需要處理滑鼠移動的話，寫code在這裡
}

void CGameStateRun::OnRButtonDown(UINT nFlags, CPoint point)  // 處理滑鼠的動作
{
	eraser.SetMovingRight(true);
}

void CGameStateRun::OnRButtonUp(UINT nFlags, CPoint point)	// 處理滑鼠的動作
{
	eraser.SetMovingRight(false);
}

void CGameStateRun::OnShow()
{
	//
	//  注意：Show裡面千萬不要移動任何物件的座標，移動座標的工作應由Move做才對，
	//        否則當視窗重新繪圖時(OnDraw)，物件就會移動，看起來會很怪。換個術語
	//        說，Move負責MVC中的Model，Show負責View，而View不應更動Model。
	//
	//
	//  貼上背景圖、撞擊數、球、擦子、彈跳的球
	//
	
//	background.ShowBitmap();			// 貼上背景圖
//	help.ShowBitmap();					// 貼上說明圖
//	gamemap.OnShow();
//	hits_left.ShowBitmap();
//	for (int i=0; i < NUMBALLS; i++)
//		ball[i].OnShow();				// 貼上第i號球
//	bball.OnShow();						// 貼上彈跳的球
//	eraser.OnShow();					// 貼上擦子
	ShowMap();
	//
	//  貼上左上及右下角落的圖
	//
//	corner.SetTopLeft(0,0);
//	corner.ShowBitmap();
//	corner.SetTopLeft(SIZE_X-corner.Width(), SIZE_Y-corner.Height());
//	corner.ShowBitmap();
//	bitmap3.ShowBitmap();
//	c_practice.OnShow();
}
CPractice::CPractice()
{
	x = y = 0;
}
void CPractice::OnMove()
{
	if (y <= SIZE_Y)
	{
		x += 3;
		y += 3;
	}
	else
	{
		x = y = 0;
	}
}
void CPractice::LoadBitmap()
{
	pic.LoadBitmap(IDB_BITMAP3);
}
void CPractice::OnShow()
{
	pic.SetTopLeft(x, y);
	pic.ShowBitmap();
}
CGameMap::CGameMap():X(20),Y(40),MW(120),MH(100)
{
	random_num = 0;
	int map_init[4][5] = {{0,0,1,0,0},
	                      {0,1,2,1,0},
						  {1,2,1,2,1},
						  {2,1,2,1,2}};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 5; j++)
			map[i][j] = map_init[i][j];
	
}
void CGameMap::LoadBitmap()
{
	blue.LoadBitmap(IDB_BLUE);
	green.LoadBitmap(IDB_GREEN);
}
void CGameMap::OnShow()
{
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 4; j++)
		{
			std::cout << "map: " + map[j][i]  << "! ";
			switch (map[j][i]) {
			case 0:
				break;
			case 1:
				blue.SetTopLeft(X + (MW*i), Y + (MH*j));
				blue.ShowBitmap();
				break;
			case 2:
				green.SetTopLeft(X + (MW*i), Y + (MH*j));
				green.ShowBitmap();
				break;
			default:
				ASSERT(0);
			}
		}
	for (int i = 0; i < random_num; i++)
	{
		bballs[i].OnShow();
	}
}
void CBouncingBall::SetXY(int x, int y)
{
	this->x = x;
	this->y = y;
}
void CBouncingBall::SetFloor(int floor)
{
	this->floor = floor;
}
void CBouncingBall::SetVelocity(int velocity)
{
	this->velocity = velocity;
	this->initial_velocity = velocity;
}
void CGameMap::InitializeBouncingBall(int ini_index, int row, int col)
{
	const int VELOCITY = 10;
	const int BALL_PIC_HEIGHT = 15;
	int floor = Y + (row + 1)*MH - BALL_PIC_HEIGHT;

	bballs[ini_index].LoadBitmap();
	bballs[ini_index].SetFloor(floor);
	bballs[ini_index].SetVelocity(VELOCITY+col);
	bballs[ini_index].SetXY(X+col*MW+MW/2,floor);
}
void CGameMap::RandomBouncingBall()
{
	const int MAX_RAND_NUM = 10;
	random_num = (rand() % MAX_RAND_NUM) + 1;

	bballs = new CBouncingBall[random_num];
	int ini_index = 0;
	for(int row=0;row<4;row++)
		for (int col = 0; col < 5; col++)
		{
			if (map[row][col] != 0 && ini_index < random_num)
			{
				InitializeBouncingBall(ini_index,row,col);
				ini_index++;
			}
		}
}
void CGameMap::OnKeyDown(UINT nChar)
{
	const int KEY_SPACE = 0x20;
	if (nChar == KEY_SPACE)
		RandomBouncingBall();
}
void CGameMap::OnMove()
{
	for (int i = 0; i < random_num; i++)
	{
		bballs[i].OnMove();
	}
}
CGameMap::~CGameMap()
{
}
}
