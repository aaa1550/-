#include <Arduino.h>
#include <IRremote.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
uint32_t fakeNow=1000;
int pins[32],pwm[32],modes[32];
int reads=0,writes=0,analogCalls=0,modeCalls=0,pulseCalls=0,delayCalls=0;
int pulsePin=-1,pulseLevel=-1;
unsigned long echoUs=1160,pulseTimeout=0;
Receiver IrReceiver;
#include "CompletedObstacleCar.ino"
using std::string;
struct Field {string key,actual,expected;};
int count=0,failed=0;
string n(long long x){return std::to_string(x);}
string f(float x){std::ostringstream s;s<<std::fixed<<std::setprecision(3)<<x;return s.str();}
string q(const string& x){string s="\"";for(char c:x){if(c=='\\'||c=='\"')s+='\\';if(c=='\n')s+="\\n";else s+=c;}return s+'\"';}
void emit(string group,string purpose,std::vector<string> inputs,std::vector<Field> fields){
 ++count;bool ok=true;for(auto& x:fields)ok&=x.actual==x.expected;if(!ok)++failed;
 std::cout<<"{\"id\":"<<q("UT-"+n(count))<<",\"function\":"<<q(group)<<",\"purpose\":"<<q(purpose)<<",\"inputs\":[";
 for(size_t i=0;i<inputs.size();i++)std::cout<<(i?",":"")<<q(inputs[i]);
 std::cout<<"],\"outputs\":[";
 for(size_t i=0;i<fields.size();i++){auto& x=fields[i];std::cout<<(i?",":"")<<"{\"key\":"<<q(x.key)<<",\"expected\":"<<q(x.expected)<<",\"actual\":"<<q(x.actual)<<"}";}
 std::cout<<"],\"result\":"<<q(ok?"合格（模擬）":"不合格（模擬）")<<"}\n";
}
void reset(){
 fakeNow=1000;for(int i=0;i<32;i++){pins[i]=LOW;pwm[i]=0;modes[i]=-1;}
 pins[0]=pins[2]=HIGH;pins[13]=HIGH;reads=writes=analogCalls=modeCalls=pulseCalls=delayCalls=0;
 pulsePin=pulseLevel=-1;pulseTimeout=0;echoUs=1160;IrReceiver=Receiver{};
 requested=STOP;holdActive=false;lastReceivedAt=1000;lastCommand=0;lastAddress=7;lastProtocol=NEC;
 appliedRight=appliedLeft=0;stopTimingActive=false;stoppedAt=0;
 SD=-1;SR1=SR2=IR=false;RM=M=0;distanceValid=false;sensorStarted=false;lastSensorAt=0;
 buzzerActive=false;buzzerStartedAt=0;
}
void active(Direction dir=FORWARD){requested=dir;holdActive=true;lastReceivedAt=fakeNow;lastCommand=IR_ADVANCE;}
void send(uint16_t command,uint8_t flags=0,uint16_t address=7,decode_type_t protocol=NEC){
 IrReceiver.pending=true;IrReceiver.decodedIRData.command=command;IrReceiver.decodedIRData.flags=flags;
 IrReceiver.decodedIRData.address=address;IrReceiver.decodedIRData.protocol=protocol;
}
int main(){
 // 元の関数：停止・操作解除・長押し期限
 for(int state=0;state<3;state++){
  reset();if(state==1){appliedRight=180;appliedLeft=-180;pwm[9]=pwm[6]=180;}
  if(state==2){stopTimingActive=true;stoppedAt=500;}
  stopMotors();emit("stopMotors","停止出力と逆転待ち時刻",{"事前右出力="+n(state==1?180:0),"事前左出力="+n(state==1?-180:0),"事前待機="+n(state==2),"事前停止時刻="+n(state==2?500:0),"時刻=1000"},{{"右PWM",n(pwm[9]),"0"},{"左PWM",n(pwm[6]),"0"},{"方向端子合計",n(pins[12]+pins[11]+pins[7]+pins[8]),"0"},{"待機フラグ",n(stopTimingActive),state?"1":"0"},{"停止時刻",n(stoppedAt),state==1?"1000":state==2?"500":"0"},{"analogWrite回数",n(analogCalls),"2"},{"digitalWrite回数",n(writes),"4"}});
 }
 reset();active(BACKWARD);appliedRight=appliedLeft=-180;cancelMovement();emit("cancelMovement","指令と長押しを解除して停止",{"requested=BACKWARD","holdActive=1","左右出力=-180"},{{"requested",n(requested),"0"},{"holdActive",n(holdActive),"0"},{"右PWM",n(pwm[9]),"0"},{"左PWM",n(pwm[6]),"0"},{"待機フラグ",n(stopTimingActive),"1"}});
 for(int age:{0,249,250,251}){
  reset();active();lastReceivedAt=fakeNow-age;checkHoldTimeout();bool stop=age>=250;
  emit("checkHoldTimeout","250ms境界",{"holdActive=1","経過ms="+n(age)},{{"holdActive",n(holdActive),stop?"0":"1"},{"requested",n(requested),stop?"0":"1"},{"停止呼出のPWM書込",n(analogCalls),stop?"2":"0"}});
 }
 reset();lastReceivedAt=0;checkHoldTimeout();emit("checkHoldTimeout","操作していない場合は変更なし",{"holdActive=0"},{{"holdActive",n(holdActive),"0"},{"analogWrite回数",n(analogCalls),"0"}});
 reset();active();lastReceivedAt=0xfffffff0U;fakeNow=234;checkHoldTimeout();emit("checkHoldTimeout","32bit時刻折り返しで250ms停止",{"最終時刻=4294967280","現在時刻=234"},{{"holdActive",n(holdActive),"0"},{"requested",n(requested),"0"}});
 int keys[]={24,82,90,8,28,255};int dirs[]={1,2,3,4,0,0};
 for(int i=0;i<6;i++){reset();emit("getDirection","操作キー変換",{"command="+n(keys[i])},{{"戻り値",n(getDirection(keys[i])),n(dirs[i])}});}
 for(int i=0;i<6;i++){
  reset();uint16_t address=i==3?8:7;decode_type_t protocol=i==1?NEC2:i==2?SONY:NEC;
  if(i==4){lastProtocol=NEC2;}
  if(i==5){lastProtocol=SONY;protocol=SONY;}
  bool got=sameRemote(address,protocol);
  emit("sameRemote","アドレスと方式の一致",{"前回アドレス=7","今回アドレス="+n(address),"前回方式="+n(lastProtocol),"今回方式="+n(protocol)},{{"戻り値",n(got),(i==2||i==3)?"0":"1"}});
 }
 // 受信：無信号、新規4方向、OK、異常、長押しと古いリピート
 reset();readRemote();emit("readRemote","無受信",{"pending=0"},{{"requested",n(requested),"0"},{"holdActive",n(holdActive),"0"},{"resume回数",n(IrReceiver.resumeCalls),"0"}});
 for(int i=0;i<4;i++){
  reset();send(keys[i]);readRemote();emit("readRemote","新規の方向キー",{"command="+n(keys[i]),"address=7","protocol=NEC"},{{"requested",n(requested),n(dirs[i])},{"holdActive",n(holdActive),"1"},{"最終受信時刻",n(lastReceivedAt),"1000"},{"記録command",n(lastCommand),n(keys[i])},{"記録address",n(lastAddress),"7"},{"resume回数",n(IrReceiver.resumeCalls),"1"}});
 }
 for(int kind=0;kind<7;kind++){
  reset();active();lastReceivedAt=800;uint8_t flags=kind==0?0:kind==1?IRDATA_FLAGS_PARITY_FAILED:kind==2?IRDATA_FLAGS_WAS_OVERFLOW:0;
  send(kind==0?28:kind==4?255:24,flags,7,kind==3?UNKNOWN:NEC);
  if(kind>=5){send(24,IRDATA_FLAGS_IS_REPEAT,kind==6?8:7,NEC2);}
  readRemote();bool stop=kind<4;
  emit("readRemote",kind==0?"OK停止":kind==1?"パリティ異常":kind==2?"受信オーバーフロー":kind==3?"未知プロトコル":kind==4?"未使用キーは延長しない":kind==5?"NEC2の同じ長押しを受理":"別アドレスの長押しを無視",{"command="+n(IrReceiver.decodedIRData.command),"flags="+n(IrReceiver.decodedIRData.flags),"address="+n(IrReceiver.decodedIRData.address),"protocol="+n(IrReceiver.decodedIRData.protocol),"最終受信=800","現在=1000","前回command=24","前回address=7"},{{"requested",n(requested),stop?"0":"1"},{"holdActive",n(holdActive),stop?"0":"1"},{"最終受信時刻",n(lastReceivedAt),kind==5?"1000":"800"},{"resume回数",n(IrReceiver.resumeCalls),"1"}});
 }
 reset();active();lastReceivedAt=750;send(24,IRDATA_FLAGS_IS_REPEAT);readRemote();emit("readRemote","期限切れ後のリピートで再開しない",{"経過=250ms","command=24","repeat=1"},{{"holdActive",n(holdActive),"0"},{"requested",n(requested),"0"}});
 reset();active();lastReceivedAt=800;send(82,IRDATA_FLAGS_IS_REPEAT);readRemote();emit("readRemote","異なる方向のリピートは延長しない",{"前回command=24","今回command=82","repeat=1"},{{"最終受信時刻",n(lastReceivedAt),"800"},{"requested",n(requested),"1"}});
 // 通常の出力と逆転待ち
 int right[]={180,-180,-180,180},left[]={180,-180,180,-180};
 for(int i=0;i<4;i++){
  reset();active(static_cast<Direction>(dirs[i]));driveMotors();emit("driveMotors","通常の4方向出力",{"requested="+n(dirs[i]),"holdActive=1","通常PWM=180"},{{"右符号付き出力",n(appliedRight),n(right[i])},{"左符号付き出力",n(appliedLeft),n(left[i])},{"右PWM",n(pwm[9]),"180"},{"左PWM",n(pwm[6]),"180"},{"analogWrite回数",n(analogCalls),"2"}});
 }
 reset();requested=FORWARD;driveMotors();emit("driveMotors","長押し無効なら停止",{"requested=FORWARD","holdActive=0"},{{"右PWM",n(pwm[9]),"0"},{"左PWM",n(pwm[6]),"0"}});
 for(int age:{0,99,100}){
  reset();active(BACKWARD);appliedRight=appliedLeft=180;driveMotors();fakeNow+=age;driveMotors();
  emit("driveMotors","逆転前100msの境界",{"元出力=前進180","指令=後退","停止後ms="+n(age)},{{"右符号付き出力",n(appliedRight),age<100?"0":"-180"},{"左符号付き出力",n(appliedLeft),age<100?"0":"-180"},{"待機フラグ",n(stopTimingActive),age<100?"1":"0"}});
 }
 // 追加した初期化とセンサー
 reset();setupObstacleDetection();emit("setupObstacleDetection","センサー用ピンの初期化",{"初期端子モード=-1"},{{"Trigモード",n(modes[10]),"1"},{"Echoモード",n(modes[3]),"0"},{"後方左モード",n(modes[0]),"0"},{"後方右モード",n(modes[2]),"0"},{"ブザー端子",n(pins[13]),"1"},{"pinMode回数",n(modeCalls),"4"},{"digitalWrite回数",n(writes),"2"}});
 unsigned long echoes[]={0,1,579,580,581,869,870,871,15000};
 string distances[]={"-1.000","0.017","9.983","10.000","10.017","14.983","15.000","15.017","258.621"};
 for(int i=0;i<9;i++){
  reset();SR1=SR2=true;echoUs=echoes[i];updateUltrasonicState();
  emit("updateUltrasonicState","10cm・15cmの境界と無応答",{"pulseIn戻りus="+n(echoes[i]),"前回SR1=1,SR2=1"},{{"距離SD",f(SD),distances[i]},{"有効距離",n(distanceValid),i?"1":"0"},{"SR1",n(SR1),i>=1&&i<=3?"1":"0"},{"SR2",n(SR2),i>=4&&i<=6?"1":"0"},{"pulseIn回数",n(pulseCalls),"1"},{"Echoピン",n(pulsePin),"3"},{"タイムアウトus",n(pulseTimeout),"15000"}});
 }
 for(int age:{59,60}){
  reset();sensorStarted=true;lastSensorAt=1000;distanceValid=true;SD=20;fakeNow+=age;updateUltrasonicState();
  emit("updateUltrasonicState","測距周期60ms",{"sensorStarted=1","経過ms="+n(age)},{{"pulseIn回数",n(pulseCalls),age==60?"1":"0"},{"最終測距時刻",n(lastSensorAt),age==60?"1060":"1000"}});
 }
 for(int l=0;l<=1;l++)for(int r=0;r<=1;r++){
  reset();pins[0]=l;pins[2]=r;updateRearInfraredState();emit("updateRearInfraredState","LOW検知・左右ORと解除",{"後方左D0="+n(l),"後方右D2="+n(r)},{{"IR",n(IR),(l==0||r==0)?"1":"0"},{"digitalRead回数",n(reads),l==0?"1":"2"}});
 }
 // 車両：停止優先、前後回避、全方向
 for(int i=0;i<4;i++){
  reset();active(static_cast<Direction>(dirs[i]));distanceValid=true;updateVehicleState();emit("updateVehicleState","障害物なしの4方向",{"requested="+n(dirs[i]),"holdActive=1","distanceValid=1","SR1=0","IR=0"},{{"RM",n(RM),n(i+1)},{"M",n(M),n(i+3)}});
 }
 for(int kind=0;kind<7;kind++){
  reset();active();distanceValid=true;
  if(kind==0){SR1=true;}
  if(kind==1){IR=true;}
  if(kind==2){SR1=IR=true;}
  if(kind==3){holdActive=false;}
  if(kind==4){requested=STOP;}
  if(kind==5){distanceValid=false;}
  if(kind==6)lastReceivedAt=750;
  updateVehicleState();int want=kind==0?1:kind==1?2:0;
  emit("updateVehicleState",kind==0?"前方で後退モード":kind==1?"後方で前進モード":kind==2?"前後同時は停止":kind==3?"長押し解除は停止":kind==4?"STOP指令は停止":kind==5?"測距失敗は停止":"測距中の期限切れを停止",{"事前requested="+n(kind==4?0:1),"事前holdActive="+n(kind!=3),"distanceValid="+n(kind!=5),"SR1="+n(SR1),"IR="+n(IR),"最終受信="+n(kind==6?750:1000),"現在=1000"},{{"M",n(M),n(want)},{"holdActive",n(holdActive),(kind==3||kind==6)?"0":"1"}});
 }
 // ブザーの時間境界と距離に応じた周期
 for(int mode=0;mode<5;mode++){
  reset();SR1=mode==1;IR=mode==2;SR2=mode>=3;SD=mode==3?15:12.5f;updateBuzzerState();
  emit("updateBuzzerState","警告開始条件",{"SR1="+n(SR1),"IR="+n(IR),"SR2="+n(SR2),"SD="+f(SD)},{{"ブザーD13",n(pins[13]),mode?"0":"1"},{"鳴動管理フラグ",n(buzzerActive),mode?"1":"0"}});
 }
 for(int age:{34,35,99,100}){
  reset();SR1=true;buzzerActive=true;buzzerStartedAt=1000;fakeNow+=age;updateBuzzerState();
  emit("updateBuzzerState","ON35msと高速周期100msの境界",{"SR1=1","経過ms="+n(age)},{{"D13",n(pins[13]),(age<35||age==100)?"0":"1"},{"周期起点",n(buzzerStartedAt),age==100?"1100":"1000"}});
 }
 for(int which=0;which<2;which++){
  reset();SR2=true;SD=which?12.5f:15;uint32_t period=which?350:600;
  buzzerActive=true;buzzerStartedAt=1000;fakeNow+=period-1;updateBuzzerState();int before=pins[13];fakeNow++;updateBuzzerState();
  emit("updateBuzzerState","距離別周期の直前と一致",{"距離cm="+f(SD),"周期ms="+n(period)},{{"周期直前D13",n(before),"1"},{"周期一致D13",n(pins[13]),"0"}});
 }
 reset();buzzerActive=true;pins[13]=LOW;updateBuzzerState();emit("updateBuzzerState","障害物解除で消音",{"SR1=0","SR2=0","IR=0","前回鳴動=1"},{{"D13",n(pins[13]),"1"},{"鳴動管理フラグ",n(buzzerActive),"0"}});
 // 回避動作：モード、出力、100ms待ち
 for(int m=0;m<=6;m++){
  reset();M=m;bool got=applyObstacleMovement();emit("applyObstacleMovement","モード別の回避と通常制御への受け渡し",{"M="+n(m)},{{"戻り値",n(got),m<=2?"1":"0"},{"右符号付き出力",n(appliedRight),m==1?"-100":m==2?"100":"0"},{"左符号付き出力",n(appliedLeft),m==1?"-100":m==2?"100":"0"},{"右PWM",n(pwm[9]),m==1||m==2?"100":"0"},{"左PWM",n(pwm[6]),m==1||m==2?"100":"0"}});
 }
 for(int age:{0,99,100}){
  reset();M=1;appliedRight=appliedLeft=180;applyObstacleMovement();fakeNow+=age;applyObstacleMovement();
  emit("applyObstacleMovement","前方回避の反転100ms境界",{"元出力=前進180","M=1","停止後ms="+n(age)},{{"右符号付き出力",n(appliedRight),age<100?"0":"-100"},{"左符号付き出力",n(appliedLeft),age<100?"0":"-100"},{"待機フラグ",n(stopTimingActive),age<100?"1":"0"}});
 }
 reset();setup();emit("setup","モーター・センサー・受信の初期化",{"起動直後"},{{"右PWM",n(pwm[9]),"0"},{"左PWM",n(pwm[6]),"0"},{"pinMode回数",n(modeCalls),"11"},{"IR初期化回数",n(IrReceiver.beginCalls),"1"},{"IR受信ピン",n(IrReceiver.beginPin),"4"},{"LEDフィードバック",n(IrReceiver.feedback),"0"},{"ブザーD13",n(pins[13]),"1"}});
 // mainシート用：loopの連携と実機とは分離した模擬結果
 for(int i=0;i<4;i++){
  reset();send(keys[i]);loop();emit("loop","4方向の全体処理順",{"command="+n(keys[i]),"Echo=1160us(20cm)","後方左右=HIGH"},{{"M",n(M),n(i+3)},{"右符号付き出力",n(appliedRight),n(right[i])},{"左符号付き出力",n(appliedLeft),n(left[i])},{"ブザーD13",n(pins[13]),"1"},{"pulseIn回数",n(pulseCalls),"1"},{"holdActive",n(holdActive),"1"}});
 }
 for(int kind=0;kind<4;kind++){
  reset();send(24);echoUs=kind==0||kind==2?580:kind==3?0:1160;if(kind==1||kind==2)pins[0]=LOW;
  loop();int want=kind==0?1:kind==1?2:0;int speed=kind==0?-100:kind==1?100:0;
  emit("loop",kind==0?"10cmで後退":kind==1?"後方検知で前進":kind==2?"前後同時停止":"無応答停止",{"command=24","echoUs="+n(echoUs),"後方左D0="+n(pins[0]),"後方右D2="+n(pins[2])},{{"M",n(M),n(want)},{"右符号付き出力",n(appliedRight),n(speed)},{"左符号付き出力",n(appliedLeft),n(speed)},{"ブザーD13",n(pins[13]),kind==3?"1":"0"}});
 }
 reset();send(24);loop();pins[0]=LOW;fakeNow+=1;loop();fakeNow+=100;lastReceivedAt=fakeNow;loop();
 int avoidance=appliedRight;pins[0]=HIGH;fakeNow+=1;loop();
 emit("loop","後方検知解除で元の操作へ戻る",{"上長押し継続","後方 HIGH→LOW→HIGH"},{{"回避時右出力",n(avoidance),"100"},{"解除後M",n(M),"3"},{"解除後右出力",n(appliedRight),"180"}});
 reset();send(24);echoUs=580;loop();fakeNow+=250;loop();emit("loop","回避中も長押し解除で停止",{"前方10cm","受信中断250ms"},{{"M",n(M),"0"},{"右PWM",n(pwm[9]),"0"},{"holdActive",n(holdActive),"0"}});
 reset();send(24);echoUs=580;loop();send(28);loop();emit("loop","回避中にOKで停止",{"前方10cm","command=28"},{{"M",n(M),"0"},{"右PWM",n(pwm[9]),"0"},{"holdActive",n(holdActive),"0"}});
 std::cerr<<"TOTAL="<<count<<" FAILED="<<failed<<"\n";return failed?1:0;
}
