/* 
 
 MX2 El Cheapo - Camera Control Functions
 modified Version of Dynamic Perception LLC's DollyShield ds_camera.ino
 (c) 2010-2011 C.A. Church / Dynamic Perception LLC
 (c) FFZ
 For more info go to http://openmoco.org or http://www.thundercorp.de/timelapse
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

/*

 ========================================
 Camera control functions
 ========================================
 
 */

void fire_camera(unsigned long exp_tm) {
  //TODO IR  

  // determine if focus pin should be brought high
  // w. the shutter pin (for some nikons, etc.)
  if (!ir_remote){   
    if( focus_shutter ) //TODO
      digitalWriteFast(FOCUS_PIN, HIGH);

    digitalWriteFast(CAMERA_PIN, HIGH);
  }
  else{
    send_ir();
  }
  // start timer to stop camera exposure
  MsTimer2::set(exp_tm, stop_camera);
  MsTimer2::start();

  // update camera currently enaged
  // (turn on bit)
  S_CAM_ENGAGED=true;
  //run_status |= B01000000;

  return;
}


void stop_camera() {

  digitalWriteFast(CAMERA_PIN, LOW);

  // we do this every time, because
  // it's possible that the flag
  // that controls whether or not to
  // trip focus w. shutter may have
  // been reset during our exposure,
  // and failing to do so would keep
  // the focus pin high for a long
  // time.

  digitalWriteFast(FOCUS_PIN, LOW);

  // turn off timer - we do this
  // after the digitalWriteFast() to minimize
  // over-shooting in case this takes some
  // unusually-long amount of time
 
  // if (ir_remote) send_ir();  
  MsTimer2::stop();

  // are we supposed to delay before allowing
  // the motors to move?  Register a timer
  // to clear out status flags, otherwise
  // just clear them out now.

  // the delay is used to prevent motor movement
  // when shot timing is controlled by the camera.
  // the post-delay should be set to an amount greater
  // than the max possible camera exposure timing

  // update camera currently engaged
 S_CAM_ENGAGED=false;// run_status &= B10111111;

  // update camera cycle complete
 S_CAM_CYCLE_COMPLETE=true; //run_status |= B00100000;

}


void camera_clear() {
  // clears out camera engaged settings
  // so that motor control and other actions can 
  // be undertaken.  Used as a timer whenever
  // a camera post delay is set.

  MsTimer2::stop(); // turn off timer

  // update camera currently engaged
 S_CAM_ENGAGED=false; //run_status &= B10111111;

  // update camera cycle complete
S_CAM_CYCLE_COMPLETE=true;  //run_status |= B00100000;

}  



void stop_cam_focus() {

  MsTimer2::stop();
  digitalWriteFast(FOCUS_PIN, LOW);
  pre_focus_clear = 2;

}

void clear_cam_focus() {
  MsTimer2::stop();
  pre_focus_clear = 4;
}

float calc_total_cam_tm() {

  // calculate total minimum time between exposures 

  byte pf_tm = 0;

  // add 100ms pre-focus tap clear value
  if( focus_tap_tm > 0 ) 
    pf_tm = focus_tap_tm + 100;

  float total = (float) ( exp_tm + pf_tm + post_delay_tm  );

  if( ! motor_sl_mod )
    total += m_sms_tm[0];

  total = total / 1000.00;

  return(total);
}

void send_ir(){
  for(unsigned int i=1;i<=seq[0];i++){
    int ir_status=0;
    int n=seq[i];
    while(n>0){
      n--;
      delayMicroseconds(oscd);
      ir_status  =  !ir_status; 
      digitalWriteFast(IR_PIN, ir_status&&(i%2));   
    }
  }
}

