<?php
/*
 * This code is part of GOsa (http://www.gosa-project.org)
 * Copyright (C) 2003-2008 GONICUS GmbH
 *
 * ID: $$Id$$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

class aclManagement extends plugin
{
  /* Plugin definitions */
  var $plHeadline     = "ACL";
  var $plDescription  = "Manage access control lists";

  /* Dialog attributes */
  var $acltabs              = NULL;
  var $ui                   = NULL;
  var $acl                  = "";
  var $DivListACL           = NULL;

  var $CopyPasteHandler;
  var $start_pasting_copied_objects = FALSE;
  var $dns = array();

  var $acl_module = array("acl","aclroles");

  function aclManagement(&$config, &$ui)
  {
    /* Save configuration for internal use */
    $this->config = &$config;
    $this->ui     = &$ui;

    /* Copy & Paste enabled ?*/
    if ($this->config->get_cfg_value("copyPaste") == "true"){
      $this->CopyPasteHandler = new CopyPasteHandler($this->config);
    }

    /* Creat dialog object */
    $this->DivListACL = new divListACL($this->config,$this);
  }


  function execute()
  {
    /* Call parent execute */
    plugin::execute();

    session::set('LOCK_VARS_TO_USE',array("/^item_selected/","/^menu_action/",
          "/^list/","/^id_/","/^list_acl_role_del/","/^list_acl_del/","/^menu_action/"));

    $smarty       = get_smarty();                 // Smarty instance
    $s_action     = "";                           // Contains the action to be taken
    $s_entry      = "";                           // The value for s_action

    /* Edit entry button pressed? */
    if( isset($_GET['act']) && $_GET['act'] == "list_edit_entry" ){
      $s_action= "edit";
      $s_entry= validate($_GET['id']);
    }

    /* Edit entry button pressed? */
    if( isset($_GET['act']) && $_GET['act'] == "list_edit_role" ){
      $s_action= "edit_role";
      $s_entry= validate($_GET['id']);
    }

    $types = array( 
                    "del"       =>"^list_acl_del",
                    "edit"      =>"^list_acl_edit",
                    "edit_role" =>"^list_acl_role_edit",
                    "copy"      =>"^copy",
                    "cut"       =>"^cut",
                    "copy_multiple"=> "^multiple_copy_objects",
                    "cut_multiple" => "^multiple_cut_objects",
                    "editPaste" =>"^editPaste",
                    "addrole"   =>"^new_acl_role");

    /* Test relevant POST values */  
    foreach($_POST as $key => $val){

      /* Get every possible POST combination and set s_action/s_entry accordingly */
      foreach($types as $act => $name){

        if (preg_match("/".$name.".*/", $key)){
          $s_action= $act;
          $s_entry= preg_replace("/".$name."_/i", "", $key);
        }
      }
    } 

    /* Remove coordinate prefix from POST, required by some browsers */
    $s_entry= preg_replace("/_.$/", "", $s_entry);

    /* Seperate possibly encoded tab and entry, default to tab "acl" */
    if(preg_match("/.*-.*/", $s_entry)){
      $s_tab= preg_replace("/^[^-]*-/i", "" ,$s_entry);
      $s_entry= preg_replace("/-[^-]*$/i", "", $s_entry);
    }else{
      $s_tab= "generic";
    }
   

    /* handle C&P from layers menu */
    if(isset($_POST['menu_action']) && preg_match("/^multiple_copy_systems/",$_POST['menu_action'])){
      $s_action = "copy_multiple";
    }
    if(isset($_POST['menu_action']) && preg_match("/^multiple_cut_systems/",$_POST['menu_action'])){
      $s_action = "cut_multiple";
    }
    if(isset($_POST['menu_action']) && preg_match("/^editPaste/",$_POST['menu_action'])){
      $s_action = "editPaste";
    }

    /* Create options */
    if(isset($_POST['menu_action']) && $_POST['menu_action'] == "new_acl_role"){
      $s_action = "new";
    }

    /* handle remove from layers menu */
    if(isset($_POST['menu_action']) && preg_match("/^remove_multiple/",$_POST['menu_action'])){
      $s_action = "del_multiple";
    }

 
    /********************
      Copy & Paste Handling  ...
     ********************/

    /* Display the copy & paste dialog, if it is currently open */
    $ret = $this->copyPasteHandling_from_queue($s_action,$s_entry);
    if($ret){
      return($ret);
    }


    /********************
     * Add new Role entry 
     ********************/

    if(($s_action == "new") && (!isset($this->acltabs->config))){

      /* Get 'dn' from posted acl, must be unique */
      $this->dn= "new";

      /* Check permissions */
      if(preg_match("/c/",$this->ui->get_permissions($this->DivListACL->selectedBase,"aclroles/aclrole"))){

        /* Register acltabs to trigger edit dialog */
        $this->acltabs= new aclroletab($this->config, NULL,$this->dn);
        $this->acltabs->set_acl_base($this->DivListACL->selectedBase);
      }else{
        msg_dialog::display(_("Permission error"), msgPool::permCreate(), ERROR_DIALOG);
      }
    }

    /********************
      Edit existing entry 
     ********************/

    if (($s_action=="edit" || $s_action=="edit_role") && (!isset($this->acltabs->config))){

      /* Get 'dn' from posted acl, must be unique */
      $this->dn= $this->list[trim($s_entry)]['dn'];

      if(in_array("gosaRole",$this->list[trim($s_entry)]['objectClass'])){
        $acl = "aclroles/aclrole";
      }else{
        $acl = "acl/acl";
      }

      /* Check permissions */
      if(preg_match("/r/",$this->ui->get_permissions($this->dn,$acl))){
      
        /* Check locking, save current plugin in 'back_plugin', so
           the dialog knows where to return. */
        if (($acl= get_lock($this->dn)) != ""){
          return(gen_locked_message ($acl, $this->dn));
        }

        /* Lock the current entry, so everyone will get the above dialog */
        add_lock ($this->dn, $this->ui->dn);

        /* Register acltabs to trigger edit dialog */ 
        if($s_action=="edit_role"){
          $this->acltabs= new aclroletab($this->config, NULL,$this->dn);
          $this->acltabs-> set_acl_base($this->dn);
        }else{
          $this->acltabs= new acltab($this->config, NULL,$this->dn);
          $this->acltabs-> set_acl_base($this->dn);
        }

        /* Set ACL and move DN to the headline */
        session::set('objectinfo',$this->dn);
      }else{
        msg_dialog::display(_("Permission error"), msgPool::permModify(), ERROR_DIALOG);
      }
    }


    /********************
      Edit canceled 
     ********************/
    if(isset($_POST['delete_lock'])){
      $this->remove_lock();
      session::un_set('objectinfo');
    }

    /* Reset all relevant data, if we get a _cancel request */
    if (isset($_POST['edit_cancel'])){
      $this->remove_lock();
      $this->acltabs= NULL;
      session::un_set('objectinfo');
    }


    /********************
      Delete entry requested, display confirm dialog
     ********************/

    /* Remove acl was requested */
    if ($s_action=="del" || $s_action == "del_multiple"){

      /* Collect entries to delete 
       */ 
      if($s_action == "del"){
        $ids = array(trim($s_entry));
      }else{
        $ids = $this->list_get_selected_items();
      }

      /* Check ACLs and collect removeable entries.
       */
      $this->ids = array();
      $this->dns = array();
      $disallowed = array();
      foreach($ids as $id){
        $dn = $this->list[$id]['dn'];
        if(in_array("gosaRole",$this->list[$id]['objectClass'])){
          $acl = $this->ui->get_permissions($dn, "aclroles/aclrole");
        }else{
          $acl = $this->ui->get_permissions($dn, "acl/acl");
        }
        if(!preg_match("/d/",$acl)){
          $disallowed[] = $dn;
        }else{
          $this->ids[$id]  = $id;
          $this->dns[$id]  = $dn;
        }
      }

      /* Display a message box containing all entries that we are not allowed to remove. 
       */
      if(count($disallowed)){
        msg_dialog::display(_("Permission"),msgPool::permDelete($disallowed),INFO_DIALOG);
      }

      /* We have at least one entry that can be removed
       */
      if(count($this->ids)){

        /* Display lock messages */
        if ($user= get_multiple_locks($this->dns)){
          return(gen_locked_message($user,$this->dns));
        }

        /* Create a readable string about what will be done.
         */
        $dns_names = array();
        foreach($this->ids as $id){

          /* Check permissions depending on the acl object type
           */
          $dn = $this->list[$id]['dn'];
          if(in_array("gosaRole",$this->list[$id]['objectClass'])){
            $acl = $this->ui->get_permissions($dn,"aclroles/aclrole");
            $name = _("ACL role");
          }else{
            $acl = $this->ui->get_permissions($dn,"acl/acl");
            $name = _("ACL");
          }
          $name = str_pad($name,10," ");
          $name = preg_replace("/ /","&nbsp;",$name);

          /* Append each entry to the displayed information dialog, 
              if we are allowed to remove the entry.
           */
          if(!preg_match("/d/",$acl)){
            msg_dialog::display(_("Permission error"), msgPool::permDelete(), ERROR_DIALOG);
            unset($this->dns[$id]);
            unset($this->ids[$id]);
            continue;
          }
          $dns_names[] = "</i><font style='font-weight:bold; font-family: courier;'>".$name."</font><i> ".LDAP::fix($dn);
        }
        if(count($this->dns)){
          add_lock($this->dns,$this->ui->dn);  
          $smarty->assign("info", msgPool::deleteInfo($dns_names));
          $smarty->assign("is_role",false);
          return($smarty->fetch(get_template_path('remove.tpl', TRUE)));
        }
      }
    }


    /********************
      Delete entry confirmed 
     ********************/

    /* Confirmation for deletion has been passed. Acl should be deleted. */
    if (isset($_POST['delete_acl_confirm'])){

      foreach($this->ids as $id){
     
        /* Depending on the type of acl we want to remove 
            create a new acl or a new gosaRole object 
         */ 
        $dn = $this->list[$id]['dn'];
        $this->dn = $dn;
        if(in_array("gosaRole",$this->list[$id]['objectClass'])){
          $acl = $this->ui->get_permissions($dn,"aclroles/aclrole");
          $this->acltabs= new aclroletab($this->config,NULL, $this->dn);
        }else{
          $acl = $this->ui->get_permissions($dn,"acl/acl");
          $this->acltabs= new acltab($this->config,NULL, $this->dn);
        }

        /* Check permissions */
        if(preg_match("/d/",$acl)){
          $this->acltabs->delete();
          unset ($this->acltabs);
          $this->acltabs= NULL;
        } else {
          msg_dialog::display(_("Permission error"),msgPool::permDelete(), ERROR_DIALOG);
          if(isset($this->ui->uid)){
            new log("security","aclroles/".get_class($this),$this->dn,array(),
                "Warning: '".$this->ui->uid."' tried to trick acl role      deletion.");
          }
        }
      }

      /* Remove lock file after successfull deletion */
      $this->remove_lock();
    }

    
    /********************
      Delete entry Canceled 
     ********************/

    /* Delete acl canceled? */
    if (isset($_POST['delete_acl_cancel'])){
      $this->remove_lock();
    }


    /********************
      Edit entry finished (Save) 
     ********************/

    /* Finish acl edit is triggered by the tabulator dialog, so
       the acl wants to save edited data. Check and save at this
       point. */
    if ((isset($_POST['edit_finish']) || isset($_POST['edit_apply'])) && (isset($this->acltabs->config))){

      /* Check tabs, will feed message array */
      $this->acltabs->save_object();
      $message= $this->acltabs->check();

      /* Save, or display error message? */
      if (count($message) == 0){

        /* Save acl data to ldap */
        if($this->acltabs->save() == 1){
          return;
        }

        if (!isset($_POST['edit_apply'])){

          /* ACl has been saved successfully, remove lock from LDAP. */
          if ($this->dn != "new"){
            $this->remove_lock();
          }

          unset ($this->acltabs);
          $this->acltabs= NULL;
          session::un_set('objectinfo');
        }
      } else {
        /* Ok. There seem to be errors regarding to the tab data,
           show message and continue as usual. */
        msg_dialog::displayChecks($message);
      }
    }


    /********************
      Display subdialog 
     ********************/


    /* Show tab dialog if object is present */
    if(isset($this->acltabs->config)){
      
      /* Save object */
      $this->acltabs->save_object();
      $display= $this->acltabs->execute();

      /* Don't show buttons if tab dialog requests this */
      if(isset($this->acltabs)){

        /* Skip displaying save/cancel if there is a sub dialog open */
        if (!isset($this->acltabs->dialog) || !$this->acltabs->dialog){
          $display.= "<p style=\"text-align:right\">\n";
//          $display.= "<input type=submit name=\"edit_finish\" style=\"width:80px\" value=\"".msgPool::okButton()."\">\n";
          $display.= "&nbsp;\n";

          /* Skip Apply if it is a new entry */
          #if ($this->dn != "new"){
          #  $display.= "<input type=submit name=\"edit_apply\" value=\"".msgPool::applyButton()."\">\n";
          #  $display.= "&nbsp;\n";
          #}

  //        $display.= "<input type=submit name=\"edit_cancel\" value=\"".msgPool::cancelButton()."\">\n";
          $display.= "</p>";
        }
      }
      return ($display);
    }
    
    /* Check if there is a snapshot dialog open */
    $base = $this->DivListACL->selectedBase;
    if($str = $this->showSnapshotDialog($base,$this->get_used_snapshot_bases(),$this)){
      return($str);
    }
  
    /* Return rendered main page */
        /* Display dialog with system list */
    $this->DivListACL->parent = $this;
    $this->DivListACL->execute();

    /* Add departments if subsearch is disabled */
    if(!$this->DivListACL->SubSearch){
      $this->DivListACL->AddDepartments($this->DivListACL->selectedBase,3,1);
    }
    $this->reload();
    $this->DivListACL->setEntries($this->list);
    return($this->DivListACL->Draw());
  }


  function reload()
  {
    /* Get divlist informations from filter part */
    $Regex      = $this->DivListACL -> Regex;
    $SubSearch  = $this->DivListACL -> SubSearch;
    $base       = $this->DivListACL -> selectedBase; 
    $Attrs      = array("ou","cn","description","gosaAclEntry","objectClass");
    $res        = array();
    $tmp        = array();                                    // Will contain temporary results 
    $ldap       = $this->config->get_ldap_link();
    $Filter     = "(&(objectClass=gosaACL)(gosaAclEntry=*)(|(cn=".$Regex.")(ou=".$Regex.")))";
    $FilterRoles= "(&(objectClass=gosaRole)(|(cn=".$Regex.")(ou=".$Regex.")))";

    /* Fetch following structures, this will be used if !$SubSearch */
    $fetch_this = array(
      "ME"      => array("TYPE" => "cat"    , "FLAGS" => GL_SIZELIMIT                ,"BASE"=>""),
      "SYSTEMS" => array("TYPE" => "search" , "FLAGS" => GL_SIZELIMIT | GL_SUBSEARCH ,"BASE"=>get_ou('systemsou')),
      "APPS"    => array("TYPE" => "search" , "FLAGS" => GL_SIZELIMIT | GL_SUBSEARCH ,"BASE"=>get_ou('applicationou')),
      "PEOPLE"  => array("TYPE" => "search" , "FLAGS" => GL_SIZELIMIT | GL_SUBSEARCH ,"BASE"=>get_people_ou()),
      "GROUPS"  => array("TYPE" => "search" , "FLAGS" => GL_SIZELIMIT | GL_SUBSEARCH ,"BASE"=>get_groups_ou()));

    /* Append the ldap base to be able to set alcs if there were currently no acls set 
     */
    if($base == $this->config->current['BASE']){
      $res[$base] = array("dn"=>$this->config->current["BASE"],"ou"=>".","objectClass"=>array("gosaACL"));
    }

    /* Subsearch ? */
    if($SubSearch){

      /* Get all object in this base */
      $Flags        = GL_SIZELIMIT | GL_SUBSEARCH;
      $fetch_base   = $base;
      $tmp  = get_list($Filter,   "acl", $fetch_base, $Attrs, $Flags);
      $tmp2 = get_list($FilterRoles,  "acl", $fetch_base, $Attrs, $Flags);
      foreach($tmp as $entry){
        $res[] = $entry;
      }
      foreach($tmp2 as $entry){
        $res[] = $entry;
      }

    }else{

      $tmp_roles = get_list($FilterRoles,  "acl", get_ou('aclroleou').$base, $Attrs,GL_SIZELIMIT);

      foreach($tmp_roles as $entry){
        $res[] = $entry;
      }

      /* Walk through all possible bases */
      foreach($fetch_this as $type => $data){

        /* Get requried attributes */
        $Flags        = $data['FLAGS'];
        $fetch_base   = $data['BASE'].$base;
        $Type         = $data['TYPE'];

        /* Check if method is cat or search */
        if($Type == "search"){
          $tmp = get_list($Filter, "acl", $fetch_base, $Attrs, $Flags);
          foreach($tmp as $entry){
            $res[$entry['dn']] = $entry;
          }
        }else{
          $ldap->cat($fetch_base,$Attrs);
          $attrs = $ldap->fetch();
          if($attrs && isset($attrs['gosaAclEntry'])){
            $re2 = preg_replace("/\*/",".*",$Regex);

            if(!isset($attrs['ou']) && !isset($attrs['cn'])){
              $namingAttr= preg_replace("/^[^=]*+=([^,]*),.*$/","\\1",$fetch_base);
              if(preg_match("/".$re2."/i",$namingAttr)){
                $res[$attrs['dn']] = $attrs;
              }
            }elseif( (isset($attrs['cn'][0]) && preg_match("/".$re2."/i",$attrs['cn'][0]))
               ||(isset($attrs['ou'][0]) && preg_match("/".$re2."/i",$attrs['ou'][0]))){
              $res[$attrs['dn']] = $attrs;
            }
          }
        }
      }
    }
    $this->list = array_values($res);
  }


  function copyPasteHandling_from_queue($s_action,$s_entry)
  {
    /* Check if Copy & Paste is disabled */
    if(!is_object($this->CopyPasteHandler)){
      return("");
    }

    $ui = get_userinfo();

    /* Add a single entry to queue */
    if($s_action == "cut" || $s_action == "copy"){

      /* Cleanup object queue */
      $this->CopyPasteHandler->cleanup_queue();
      $dn = $this->list[$s_entry]['dn'];

      /* We can only copy & cut roles */ 
      if(isset($this->list[$s_entry]['objectClass']) && in_array("gosaRole",$this->list[$s_entry]['objectClass'])){
        if($s_action == "copy" && $ui->is_copyable($dn,"aclroles","aclrole")){ 
          $this->CopyPasteHandler->add_to_queue($dn,$s_action,"aclroletab","ACLROLETAB","aclroles");
        }
        if($s_action == "cut" && $ui->is_cutable($dn,"aclroles","aclrole")){
          $this->CopyPasteHandler->add_to_queue($dn,$s_action,"aclroletab","ACLROLETAB","aclroles");
        }
      }
    }

    /* Add entries to queue */
    if($s_action == "copy_multiple" || $s_action == "cut_multiple"){

      /* Cleanup object queue */
      $this->CopyPasteHandler->cleanup_queue();

      /* Add new entries to CP queue */
      foreach($this->list_get_selected_items() as $id){
        $dn = $this->list[$id]['dn'];

        if(isset($this->list[$id]['objectClass']) && in_array("gosaRole",$this->list[$id]['objectClass'])){

          if($s_action == "copy_multiple" && $ui->is_copyable($dn,"aclroles","aclrole")){ 
            $this->CopyPasteHandler->add_to_queue($dn,"copy","aclroletab","ACLROLETAB","aclroles");
          }
          if($s_action == "cut_multiple" && $ui->is_cutable($dn,"aclroles","aclrole")){
            $this->CopyPasteHandler->add_to_queue($dn,"cut","aclroletab","ACLROLETAB","aclroles");
          }
        }
      }
    }

    /* Start pasting entries */
    if($s_action == "editPaste"){
      $this->start_pasting_copied_objects = TRUE;
    }
    /* Return C&P dialog */
    if($this->start_pasting_copied_objects && $this->CopyPasteHandler->entries_queued()){

      /* Get dialog */
      $this->CopyPasteHandler->SetVar("base",$this->DivListACL->selectedBase);
      $data = $this->CopyPasteHandler->execute();

      /* Return dialog data */
      if(!empty($data)){
        return($data);
      }
    }

    /* Automatically disable status for pasting */
    if(!$this->CopyPasteHandler->entries_queued()){
      $this->start_pasting_copied_objects = FALSE;
    }
    return("");
  }


  function list_get_selected_items()
  {
    $ids = array();
    foreach($_POST as $name => $value){
      if(preg_match("/^item_selected_[0-9]*$/",$name)){
        $id   = preg_replace("/^item_selected_/","",$name);
        $ids[$id] = $id;
      }
    }
    return($ids);
  }


  function remove_lock()
  {
    /* Remove acl lock if a DN is marked as "currently edited" */
    if (isset($this->acltabs->dn)){
      del_lock ($this->acltabs->dn);
    }
    del_lock ($this->dn);
    del_lock ($this->dns);
  }


  function save_object()
  {
    /* Handle divlist filter && department selection*/
    if(!is_object($this->acltabs)){
      $this->DivListACL->save_object();
    }
    if(is_object($this->CopyPasteHandler)){
      $this->CopyPasteHandler->save_object();
    }
  }

  /* A set of disabled and therefore overloaded functions. They are
     not needed in this class. */
  function remove_from_parent() { } 
  function check() { } 

  function save() {
    echo "SAVE ACL";
  } 

  function adapt_from_template($dn, $skip= array()) { } 
  function password_change_needed() { } 


  /* Return departments, that will be included within snapshot detection */
  function get_used_snapshot_bases()
  {
    return(array());
  }  

} 
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>