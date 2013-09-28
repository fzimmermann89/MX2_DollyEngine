/* 
 
 El Cheapo Dollyshield - UI Functions
 modified Version of Dynamic Perception LLC's DollyShield ds_ui.ino
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
 Core UI functions
 ========================================
 
 */


void init_user_interface() {

  pinMode(LCD_BKL, OUTPUT);

  // turn on lcd backlight
  analogWrite(LCD_BKL, 255);

  // init lcd to 16x2 display
  lcd.begin(16, 2);
  lcd.setCursor(0,0);

  // clear and turn on autoscroll
  lcd.clear();
  //lcd.autoscroll();

  // banner
  show_banner();
  
  // set the update screen flag (draw main
  // screen)
  ui_ctrl_flags |= UI_UPDATE_DISP;

  delay(1500);

}


void check_user_interface() {


  // turn off/on lcd backlight if needed
  if( (ui_ctrl_flags & UI_LCD_BKL_ON) && (lcd_dim_tm>0) && (input_last_tm < (millis() - (lcd_dim_tm * 1000))) ) {
		//unset flag
    ui_ctrl_flags &= ~UI_LCD_BKL_ON;
    //disable display if blank lcd is enabled
    if( blank_lcd ) 
      lcd.noDisplay();
    //disable backlight  
    alt_io_display_set(0);
    
  }
  else if( (! (ui_ctrl_flags & UI_LCD_BKL_ON)) && (input_last_tm > millis() - (lcd_dim_tm * 1000)) ) {
    //set flag
    ui_ctrl_flags |= UI_LCD_BKL_ON;
    //enable display and backlight
    lcd.display();
    alt_io_display_set (lcd_bkl);
  }

  // if we're set to update the display
  // (on-demand or once a second when not
  // in a menu)
  if( ui_ctrl_flags & UI_UPDATE_DISP  ||
    ( (ui_update_tm < millis() - 1000) && ! (ui_ctrl_flags & UI_SETUP_MENU ) ) ) {

    // determine whether to show home or manual screen      
    if( ! ( ui_ctrl_flags & UI_MANUAL_MODE  ) ) {
      show_home();
    }

    else {
      show_manual();
    }
		//confirm that we have updated the display
    ui_ctrl_flags &= ~UI_UPDATE_DISP ;
    ui_update_tm = millis();
  }



  // make sure to turn off motor if in manual
  // control and no button is held  
  byte held = ui_button_check();
  if( ui_ctrl_flags & UI_MANUAL_MODE  && held == false &&  S_MOT_RUNNING)
    motor_control( false);


}


byte ui_button_check() {
//TODO WTF??
  static byte hold_but_cnt = 0;

  get_button_pressed();

  boolean held = false;

  for( byte i = BUT0; i <= BUT4; i++) {
    byte bt_press = is_button_press( i );

    if( bt_press == 0 )
      continue;

    if(  bt_press == 1 && millis() - input_last_tm > HOLD_BUT_MS ) {
      // button is pressed        

      hold_but_cnt  = 0;
      inp_val_mult  = 1;
      input_last_tm = millis();
      handle_input(i, false);

    }
    else if( bt_press == 2 ) {
      held = true;
      // button being held
      if( hold_but_tm <= millis() - HOLD_BUT_MS) {
        hold_but_tm   = millis();
        input_last_tm = millis();

        handle_input(i, true);

        hold_but_cnt++;

        if( hold_but_cnt >= 8 ) {
          inp_val_mult = inp_val_mult >= 1000 ? 1000 : inp_val_mult * HOLD_BUT_VALINC;
          hold_but_cnt = 0;
        }

      }

    } // end else if button press state == 2

  } // end for loop

  return(held);

}


void get_button_pressed() {
  // see which buttons are pressed
  // buttons are on one analog pin, the value
  // determines which button, if any is pressed

    // read analog input
  int val_read = analogRead(BUT_PIN - 14);

  // don't let it flip in a single read
  if( abs(last_but_rd - val_read) > BUT_THRESH ) {
    last_but_rd = val_read; 
    button_pressed = 0;
    return;
  }

  if( val_read > (BUT0_VAL - BUT_THRESH) && val_read < (BUT0_VAL + BUT_THRESH) ) {
    button_pressed = BUT0;
  }
  else if( val_read > (BUT1_VAL - BUT_THRESH) && val_read < (BUT1_VAL + BUT_THRESH) ) {
    button_pressed = BUT1;
  }
  else if( val_read > (BUT2_VAL - BUT_THRESH) && val_read < (BUT2_VAL + BUT_THRESH) ) {
    button_pressed = BUT2;
  }
  else if( val_read > (BUT3_VAL - BUT_THRESH) && val_read < (BUT3_VAL + BUT_THRESH) ) {
    button_pressed = BUT3;
  }
  else if( val_read > (BUT4_VAL - BUT_THRESH) && val_read < (BUT4_VAL + BUT_THRESH) ) {
    button_pressed = BUT4;
  }
  else {
    button_pressed = 0;
  }


}


byte is_button_press(byte button) {

  // determine if the given button was
  // pressed, held, or is neither

  static byte button_was = 0;

  // if the button is set as 'active'
  if( button_pressed == button ) {
    // if we have already registered a press without
    // registering a non-press
    if( button_was ) {
      // increase 'skip hold count'
      return(2);
    }
    // button was not previous pressed...
    button_was = button;
    hold_but_tm = millis();
    return(1);
  }

  // if button set as inactive

  // if button was previously set as active,
  // register previous state as inactive
  if( button_was == button ) {
    button_was = 0;
    // set button as not currently pressed
    button_pressed = 0;
  }

  return(0);
}


byte get_menu( byte mnu, byte pos ) {

  // where is our target menu when 
  // mnu.pos is pressed?  
  // MainMenu is menu 0, its submenus are menus 1-5. 
  // Special return codes for advanced, calibration, manual move and input.

  if (mnu == MENU_MAIN) return (pos+1);   //in Main Menu, 0 is mainmenu, so return pos +1
  else if (mnu == MENU_MOTOR && pos == 7) return MENU_ADVANCED; //Advanced Motor Menu
  else if (mnu == MENU_ADVANCED && pos == 0 ) return MENU_CALIBRATION; //Calibration in Adv. Menu selected
  else if (mnu == MENU_MOVEMENT) return MENU_MANUAL; 
  else return(MENU_INPUT); //No Submenu, Input Menu.

}

/*
   handle user input
 */

void handle_input( byte button, boolean held ) {

  // do what needs to be done when whatever
  // button is hit

  if( button == BUT_CT ) {
    // call center button function
    ui_button_center(held);    
  }

  else if( button == BUT_DN ) {
    ui_button_down(held);
  }

  else if( button == BUT_UP ) {
    ui_button_up(held);
  }

  else if( button == BUT_RT ) {
    ui_button_rt(held);
  }  
  else if( button == BUT_LT ) {
    ui_button_lt(held);
  }


  return;


}


// button handlers


void ui_button_center( boolean held ) {
  // center button

  

  if( ui_ctrl_flags & UI_CALIBRATE_MODE  ) {
	// on calibration screen 
	//TODO
    if( cal_flags & CAL_DONE ) {
      // completed calibrating
      cal_flags &= ~CAL_DONE;
      cal_flags &= ~CAL_CALIBRATING;
      show_calibrate();
      return;
    }  
    else if( cal_flags & UI_CALIBRATE_MODE ) {
      // in calibrating input

      if( held == true )
      {
        //abort calibrating //TODO
        return;
        }

      cal_flags|=CAL_STEP_DONE;
      return;
    }

    execute_calibrate();
    return;
  }


  if( main_scr_input > 0 ) {
    // make sure to abort main screen input
    lcd.noBlink();
    main_scr_input = 0;
  }


  // if in manual control: center exits manual mode
  if( ui_ctrl_flags & UI_MANUAL_MODE ) {
    // clear out manual ctrl flag
    ui_ctrl_flags &= ~UI_MANUAL_MODE;

    // resume back to setup menu
    ui_ctrl_flags |= UI_SETUP_MENU;
    cur_menu = MENU_MOVEMENT; // show movement menu again
    draw_menu(0, false);
    return;
  }

 // not in any setup menu, enter setup menu
  if( ! (ui_ctrl_flags & UI_SETUP_MENU) ) {
    ui_ctrl_flags |= UI_SETUP_MENU;
    cur_menu = MENU_MAIN;
    draw_menu(0,false);
  }
  else {
    // in a setup menu, find
    // the next menu to go to
    byte new_menu = get_menu(cur_menu, cur_pos);
    if( new_menu == MENU_CALIBRATION || new_menu==MENU_MANUAL ) {
      // if drawing motor manual screen or in calibration screen...
      get_value(cur_menu, cur_pos, false);
      return;
    }

    if( new_menu == MENU_INPUT && ! (ui_ctrl_flags & UI_VALUE_ENTRY) )  {
      // this is not a menu, but an input of some
      // sort

      draw_menu(3,true); //TODO
      return;
    }
    else if( ui_ctrl_flags & UI_VALUE_ENTRY ) {
      // exiting out of value entry (save...)
      // go to previous menu

      // clear the cursor position
      cur_inp_pos = 0;

      // read value back from input
      get_value(cur_menu, cur_pos, true);

      // reset the float tenths (vs 100ths) parameter
      ui_float_tenths = false;

      // clear in value setting flag
      ui_ctrl_flags &= ~UI_VALUE_ENTRY ;
      // and the flag indicating that
      // we've already displayed this value
      ui_ctrl_flags &= ~UI_DRAWN_INITAL_VALUE;
      draw_menu(0,false);
    }
    else {

      // entering another menu

      // record the last menu we were at
      push_menu(cur_menu);

      // clear in value setting flag
      ui_ctrl_flags &= ~UI_VALUE_ENTRY;
      cur_pos=0;
      cur_menu = new_menu;
      draw_menu(0,false);
    }


  }
}


void ui_button_down( boolean held ) {

  // on calibration screen //TODO

  if( ui_ctrl_flags & UI_CALIBRATE_MODE  ) {

    if( cal_flags & CAL_CALIBRATING ) {
      // in calibrating settings
      move_val(false);
      update_cal_screen();
      return;
    }
    //TODO
    // m_cur_cal = m_cur_cal > 0 ? m_cur_cal - 1 : 0;
    show_calibrate();

    return;
  }


  // if in manual motor mode...
  if( ui_ctrl_flags & UI_MANUAL_MODE) {
    motor_speed_adjust( -1 * inp_val_mult, true);
    show_manual();
    return;
  }

  // if not currently in setup menus, or
  // modifying a main screen value
  // then do nothing
  if( ! (ui_ctrl_flags & UI_SETUP_MENU) && main_scr_input == 0 )
    return;

  if( main_scr_input > 0 ) {
    move_val(false);
    // save present value
    get_mainscr_set(main_scr_input, true);
    // set screen to update
    ui_ctrl_flags |= UI_UPDATE_DISP ;
  }
  else if( ui_ctrl_flags & UI_VALUE_ENTRY ) {
    // entering a value
    move_val(false);
    draw_menu(3,true);
  }
  else {
    // moving to next menu item
    draw_menu(2,false);
  }

}

void ui_button_up( boolean held ) {

  // on calibration screen //TODO
  if( ui_ctrl_flags &  UI_CALIBRATE_MODE ) {
    if( cal_flags & CAL_CALIBRATING ) {
      // in calibrating settings
      move_val(true);
      update_cal_screen();
      return;
    }
    //TODO
    //m_cur_cal = m_cur_cal > 1 ? 2 : m_cur_cal + 1;
    show_calibrate();    
    return;
  }

  // if in manual motor mode...
  if( ui_ctrl_flags & UI_MANUAL_MODE ) {
    motor_speed_adjust(1 + inp_val_mult, true);
    show_manual();
    return;
  }

  // if not currently in setup menus, or
  // modifying a main screen value
  // then do nothing
  if( ! (ui_ctrl_flags & UI_SETUP_MENU ) && main_scr_input == 0 )
    return;

  if( main_scr_input > 0 ) {
    move_val(true);
    // save present value
    get_mainscr_set(main_scr_input, true);

    // set screen to update
    ui_ctrl_flags |= UI_UPDATE_DISP;
  }
  else if( ui_ctrl_flags & UI_VALUE_ENTRY ) {
    // entering a value
    move_val(true);
    draw_menu(3,true);
  }
  else {
    draw_menu(1,false);
  }

}

void ui_button_rt( boolean held ) {

  // clear out calibration screen value, if
  // set
    ui_ctrl_flags &= ~UI_CALIBRATE_MODE ;

  // if in manual control
  if( ui_ctrl_flags & UI_MANUAL_MODE ) {
    if( held == true ) {
      // set motor direction
      motor_dir(false);
      //enabled motors if not already running
      if( ! (S_MOT_RUNNING) )
        motor_control(true);
    }
    show_manual();


    return;
  }  


  if( ! (ui_ctrl_flags & UI_SETUP_MENU) ) {
    // we're on main screen, rt switches value we can
    // adjust
    main_screen_select(true);
    return;
  } 

}


void ui_button_lt(boolean held) {
  if( ui_ctrl_flags & UI_MANUAL_MODE ) {
    //in manual motor control
    DEBUG("manmov lt");
    if( held == true ) { 
      DEBUG("held");       
      // change motor direction
      motor_dir(true);
      // get motor moving (if not already)
      if (!(S_MOT_RUNNING)) motor_control(true);
      show_manual();
    }
  }  

  else if( ! (ui_ctrl_flags & UI_SETUP_MENU ) ) {
    // we're on main screen, lt switches value we can
    // adjust
    main_screen_select(false);
  }

  else if( ui_ctrl_flags & UI_VALUE_ENTRY ) {
    // we're in a value entry mode.  Exit
    // entry without saving the value

    // clear in value setting flag
    ui_ctrl_flags &= ~UI_VALUE_ENTRY; 
    // and the flag indicating that
    // we've already displayed this value
    ui_ctrl_flags &= ~UI_DRAWN_INITAL_VALUE;        
    // reset the float tenths (vs 100ths) parameter
    ui_float_tenths = false;

    draw_menu(0,false);
  }

  else {
    // draw previous menu
    if( cur_menu == MENU_MAIN ) { 
    // we're at the highest menu, back to main screen 
    
    // clear setup flag
    ui_ctrl_flags &= ~UI_SETUP_MENU;
    // indicate display needs updating
    ui_ctrl_flags |= UI_UPDATE_DISP;
    // clear out list of menus
    flush_menu();
    }
    else {
    // a parent menu can be drawn
    cur_menu = pop_menu();
    cur_pos = 0; 
    draw_menu(0,false);
    }
  }
}


/* 
 Draw screens
 */


void draw_menu(byte dir, boolean value_entry) {

  // turn off blinking, if on...
  lcd.noBlink();

  boolean draw_all = false;

  // determine the direction we are going, up/down (1/2),
  // draw all/new menu (but don't move position) (else)

 if( dir == 2 ) {
    // down
    cur_pos= cur_pos<max_menu[cur_menu]?cur_pos+1:0;
  }  
  else if( dir == 1 ) {
    // up
    cur_pos= cur_pos==0?max_menu[cur_menu]:cur_pos-1;
   
  }
  else {
    // draw new menu (from top)
    draw_all = true;
  }

  
  switch( cur_menu ) {
    //draw the menu
  case MENU_MAIN:
    draw_values(menu_str, draw_all, value_entry);
    break;

  case MENU_MOVEMENT:
    draw_values(mov_str, draw_all, value_entry);
    break;

  case MENU_MOTOR:
    draw_values(motor_str, draw_all, value_entry);
    break;

  case MENU_CAM:
    draw_values(cam_str, draw_all, value_entry);
    break;

  case MENU_SETUP:
    draw_values(set_str, draw_all, value_entry);
    break;
    
  case MENU_ABOUT:
    show_about();
    break;
    
  case MENU_ADVANCED:
    draw_values(motor_adv_str, draw_all, value_entry);

  default: 
    return;  
  }


}


void draw_values(const char* const these[], boolean draw_all, boolean value_entry) {
    bool updown=cur_pos%2;             //even->up, odd->down
    uint8_t first_item=(cur_pos&0xFE); //first item to be drawn, round down to even number
  if (!value_entry) {
    // clear out in value entry setting, if set
    ui_ctrl_flags &= ~UI_VALUE_ENTRY;
  
    if (cur_first_item!=first_item||draw_all){
      //draw new screen
      cur_first_item=first_item;  
      lcd.clear();    
      // clear out lcd buffer and copy menu entry to be displayed
      memset(lcd_buf, ' ', sizeof(char) * MAX_LCD_STR);
      strcpy_P(lcd_buf, (char*) pgm_read_word(&(these[cur_first_item])));
      
      //draw arrows
      if (cur_first_item>0){
        //there is one screen up
        lcd.setCursor(0,0);
        lcd.write(CHAR_UP);
      }
      if(  cur_first_item+ 2 <= max_menu[cur_menu] ){
        //there is one screen down,
        lcd.setCursor(0,1);
        lcd.write(CHAR_DOWN);
      }
      lcd.setCursor(2,0);
      lcd.print(lcd_buf);
      
      if( cur_first_item + 1 <= max_menu[cur_menu] ) {
        //there is a second item to be drawn
        memset(lcd_buf, ' ', sizeof(char) * MAX_LCD_STR);
        strcpy_P(lcd_buf, (char*)pgm_read_word(&(these[cur_first_item + 1])));
        lcd.setCursor(2,1);
        lcd.print(lcd_buf);
      }
    }
   
    //draw/delete cursor
    lcd.setCursor(1,0);
    lcd.print(cursor[updown]);
    lcd.setCursor(1,1);
    lcd.print(cursor[!updown]);
  }
  else {
    //value entry
    //write strings to screen
    lcd.clear();
    memset(lcd_buf, ' ', sizeof(char) * MAX_LCD_STR);
    strcpy_P(lcd_buf, (char*)pgm_read_word(&(these[cur_pos])));      
    lcd.setCursor(1,0);
    lcd.print(lcd_buf);
    lcd.print('?');
    lcd.setCursor(0,1);
    lcd.write(CHAR_UPDOWN);
    lcd.print(' ');
    //set entry flag
    ui_ctrl_flags |= UI_VALUE_ENTRY;
    if(! ( ui_ctrl_flags & UI_DRAWN_INITAL_VALUE ) ) {
      // have just drawn this value
      // place value from variable into
      // temporary buffer
      get_value(cur_menu, cur_pos, false);
      ui_ctrl_flags |= UI_DRAWN_INITAL_VALUE;
    }

    // display the correct current
    // temporary input value
    switch(ui_type) {
    case INPUT_IO:
      strcpy_P(lcd_buf, (char*) pgm_read_word(&(io_strings[cur_inp_int])));
      lcd.print(lcd_buf);
      break;
    case INPUT_FLOAT:
      lcd.print(cur_inp_float, (byte) 2);
      break;
    case INPUT_ONOFF:
      if (cur_inp_bool == true) {
        lcd.print("On");
      } 
      else {
        lcd.print("Off");
      }
      break;
     case INPUT_OKCANCEL:
      if (cur_inp_bool == true) {
        lcd.print(F("OK"));
      } 
      else {
        lcd.print(F("Cancel"));
      }
      break;  
    case INPUT_LTRT:
      if (cur_inp_bool == true) {
        lcd.print(F("Rt"));
      } 
      else {
        lcd.print(F("Lt"));
      }
      break;
    case INPUT_CMPCT:
      if (cur_inp_bool == true) {
        lcd.print(F("CPM"));
      } 
      else {
        lcd.print(F("PCT"));
      }
      break;
    case INPUT_CONTSMS:
      if (cur_inp_bool == MODE_CONT) {
        lcd.print(F("Cont."));
      } 
      else {
        lcd.print("SMS");
      }
      break;
    case INPUT_SLOT:
      lcd.print("Slot ");
      if( cur_inp_int == 0 )      lcd.print("A");
      else if( cur_inp_int == 1 ) lcd.print("B");
      else                        lcd.print("C");
      
      break;
    case INPUT_SHUTTER:
      strcpy_P(lcd_buf, (char*) pgm_read_word(&(shutter_strings[cur_inp_int])));
      lcd.print(lcd_buf);
      break;  
    default:
      lcd.print(cur_inp_int);
      return;
    }
  }
} 
 


/* 
 
 Menu history functions
 
 */

void push_menu(byte this_menu) { 
  // push the given entry to the end of the list
  for( byte i = 0; i < sizeof(hist_menu) / sizeof(hist_menu[0]); i++) {
    if( hist_menu[i] == 0 ) {
      hist_menu[i] = this_menu;
    }
  }
}

byte pop_menu() {
  byte bk_menu = 0;
  for( byte i = sizeof(hist_menu) / sizeof(hist_menu[0]); i > 0 ; i--) {
    if( hist_menu[i-1] != 0 ) {
      bk_menu = hist_menu[i-1];
      hist_menu[i-1] = 0;
    }
  }
  return(bk_menu);
}

void flush_menu() {
  memset(hist_menu, 0, sizeof(hist_menu) / sizeof(hist_menu[0]));
}




