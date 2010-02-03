<?php
/*
   This code is part of GOsa (https://gosa.gonicus.de)
   Copyright (C) 2003  Cajus Pollmeier

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

/* Remove locks created by this plugin
*/
$lock_msg ="";
if ($remove_lock){
  if(session::is_set('macroManagement')){
      $macl = session::get('macroManagement');
      $macl->remove_lock();
  }
}

/* Remove this plugin from session
*/
if ( $cleanup ){
  session::un_set('macroManagement');
}else{
  /* Reset requested? */
  if (isset($_GET['reset']) && $_GET['reset'] == 1){
    session::un_set ('macroManagement');
  }

  /* Create macroManagement object on demand */
  if (!session::is_set('macroManagement')){
    session::set('macroManagement',new goFonMacro ($config, $ui));
  }

  /* Get object */
  $macroManagement = session::get('macroManagement');
  $macroManagement->save_object();
  $output= $macroManagement->execute();

  /* Page header*/
  if (get_object_info() != ""){
    $display= print_header( get_template_path('plugins/gofon/images/macros.png'), 
                            _("Phone macro management"), 
                            "<img alt=\"\" align=\"middle\" src=\"".get_template_path('images/lists/locked.png')."\"> 
                            ".LDAP::fix(get_object_info()));
  } else {
    $display= print_header(get_template_path('plugins/gofon/images/macros.png'), _("Phone macro management"));
  }

  /* Show and save dialog */
  $display.= $output;
  session::set('macroManagement',$macroManagement);
}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>