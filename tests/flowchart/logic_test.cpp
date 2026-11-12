#include <assert.h>
#include <stdio.h>
#include "FlowchartLogic.h"
using namespace flowcar;
int main(){
 assert(classifyFront(0)==INVALID);assert(classifyFront(301)==EMERGENCY);
 assert(classifyFront(302)==CAUTION);assert(classifyFront(905)==CAUTION);
 assert(classifyFront(906)==CLEAR);
 Controller c;Sensors clear={CLEAR,false};auto o=update(c,clear,STOP,false,0,0);assert(o.right==0);
 o=update(c,clear,FORWARD,true,1,1);assert(o.right==50&&o.left==50);
 o=update(c,{CAUTION,false},FORWARD,true,1,50);assert(o.right==0&&c.phase==BRAKING);
 o=update(c,{CAUTION,false},FORWARD,true,1,149);assert(o.right==0);
 o=update(c,{CAUTION,false},FORWARD,true,1,150);assert(o.right==-10&&o.left==-10);
 o=update(c,{CAUTION,true},FORWARD,true,1,151);assert(o.right==0&&o.m==7);
 o=update(c,clear,FORWARD,true,1,200);assert(o.right==0);
 o=update(c,clear,FORWARD,true,2,201);assert(o.right==50);
 o=update(c,{EMERGENCY,false},FORWARD,true,2,210);assert(o.right==0&&o.m==1);
 o=update(c,clear,FORWARD,true,2,211);assert(o.right==0);
 o=update(c,clear,BACKWARD,true,3,212);assert(o.right==-30);
 o=update(c,{CLEAR,true},BACKWARD,true,3,220);assert(o.right==0);
 o=update(c,{CLEAR,true},BACKWARD,true,3,320);assert(o.right==10&&o.left==10);
 o=update(c,{CLEAR,true},STOP,true,4,321);assert(o.right==0);
 o=update(c,clear,LEFT,true,5,330);assert(o.right==30&&o.left==-30);
 o=update(c,clear,RIGHT,true,6,340);assert(o.right==-30&&o.left==30);
 o=update(c,{INVALID,false},RIGHT,true,6,350);assert(o.right==0&&o.m==8);
 o=update(c,clear,RIGHT,true,6,351);assert(o.right==0);
 o=update(c,clear,FORWARD,true,7,360);assert(o.right==50);
 o=update(c,clear,FORWARD,false,7,710);assert(o.right==0);
 // 回避完了では旧操作で再始動しない。
 o=update(c,clear,FORWARD,true,8,720);assert(o.right==50);
 o=update(c,{CAUTION,false},FORWARD,true,8,730);assert(o.right==0);
 o=update(c,{CAUTION,false},FORWARD,true,8,830);assert(o.right==-10);
 o=update(c,clear,FORWARD,true,8,850);assert(o.right==0&&c.phase==WAIT_NEW);
 o=update(c,clear,FORWARD,true,8,851);assert(o.right==0);
 // 回避上限は時計の折り返しをまたいでも有効。
 Controller w;o=update(w,clear,FORWARD,true,1,0xffffff00U);
 o=update(w,{CAUTION,false},FORWARD,true,1,0xffffff10U);
 o=update(w,{CAUTION,false},FORWARD,true,1,0xffffff74U);assert(o.right==-10);
 o=update(w,{CAUTION,false},FORWARD,true,1,uint32_t(0xffffff10U+1500U));
 assert(o.right==0&&w.phase==WAIT_NEW);
 puts("PASS: boundary, speeds, braking, front/rear avoidance, STOP, simultaneous obstacles, sensor failure, link loss, rearm, timeout and clock wrap");
}
