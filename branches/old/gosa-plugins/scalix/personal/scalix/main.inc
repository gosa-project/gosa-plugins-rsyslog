<?php
/*
   This code is part of GOsa (https://gosa.gonicus.de)
   Copyright (C) 2006  Jörn Dreyer, based on work by
                       Cajus Pollmeier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

if (!$remove_lock){
  /* Reset requested? */
  if (isset($_POST['edit_cancel']) || 
      (isset($_GET['reset']) && $_GET['reset'] == 1)){
    del_lock ($ui->dn);
    session::un_set ('edit');
    session::un_set ('scalixAccount');
  }

  /* Create mail object on demand */
  if (!session::is_set('scalixAccount') || (isset($_GET['reset']) && $_GET['reset'] == 1)){
    $scalixAccount= new scalixAccount ($config, $ui->dn);
    $scalixAccount->enable_CSN_check();
    $scalixAccount->set_acl_base($ui->dn);
    $scalixAccount->set_acl_category("users");
    session::set('scalixAccount',$scalixAccount);
  }
  $scalixAccount = session::get('scalixAccount');

  /* save changes back to object */
  if (session::is_set('edit')){
    $scalixAccount->save_object ();
  }

  /* Enter edit mode? */
  if (isset($_POST['edit'])){

    /* Check locking */
    if (($username= get_lock($ui->dn)) != ""){
      session::set('back_plugin',$plug);
      session::set('LOCK_VARS_TO_USE',array("/^edit$/","/^plug$/"));
      $lock_msg =  gen_locked_message ($username, $ui->dn);

    }else{

      /* Lock the current entry */
      add_lock ($ui->dn, $ui->dn);
      session::set('dn',$ui->dn);
      session::set('edit',TRUE);
    }
  }

  /* save changes to LDAP and disable edit mode */
  if (isset($_POST['edit_finish'])){

    /* Perform checks */
    $message= $scalixAccount->check ();

    /* No errors, save object */
    if (count ($message) == 0){
      $scalixAccount->save ();
      gosa_log ("User/scalix object'".$ui->dn."' has been saved");
      $scalixAccount->acl= "#none#";
      del_lock ($ui->dn);
      session::un_set ('edit');

      /* Remove object */
      session::un_set ('scalixAccount');
    } else {
      /* Errors found, show message */
      msg_dialog::displayChecks($message);
    }
  }

  /* Execute formular */
  $display= $scalixAccount->execute ();

  /* Store changes  in session */
  if (session::is_set('edit')){
    session::set('scalixAccount',$scalixAccount);
  }

 /* Show page footer depending on the mode */
  if ($scalixAccount->is_account){

    $display.= "<p class=\"plugbottom\">";

    /* Are we in edit mode? */
    if (session::is_set('edit')){
      $display.= "<input type=submit name=\"edit_finish\" value=\"".msgPool::saveButton()."\">\n";
      $display.= "&nbsp;";
      $display.= "<input type=submit name=\"edit_cancel\" value=\"".msgPool::cancelButton()."\">\n";
      $info= "<img alt=\"\" align=\"middle\" src=\"".get_template_path('images/lists/locked.png').
             "\"> ".$ui->dn."&nbsp;";
    } else {
      $info= "<img alt=\"\" align=\"middle\" src=\"".get_template_path('images/lists/unlocked.png').
             "\"> ".$ui->dn."&nbsp;";

      if(preg_match("/r/",$ui->get_permissions($ui->dn,"users/scalixAccount"))){
        $info.= "<img alt=\"\" align=\"middle\" src=\"".get_template_path('images/lists/on.png')."\"> ".
                _("Click the 'Edit' button below to change informations in this dialog");

        $display.= "<input type=submit name=\"edit\" value=\"".msgPool::editButton()."\">\n";
      }
      $display.= "<input type=\"hidden\" name=\"ignore\">\n";
    }
    $display.= "</p>\n";
  }


  $info= "";

  /* Page header*/
  $display= print_header(get_template_path('plugins/scalix/images/plugin.png'), _("Scalix settings"), $info).$display;

}

// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>