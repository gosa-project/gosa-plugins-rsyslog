<?php

class divListMimeTypes extends MultiSelectWindow
{
  /* Current base */
  var $selectedBase       = "";
  var $departments        = array();
  var $parent               ;
  var $ui                   ;

  /* Regex */
  var $Regex              = "*";

  /* Subsearch checkbox */
  var $SubSearch;

  function divListMimeTypes (&$config, &$parent)
  {
    /* Create divlist and setup */
    MultiSelectWindow::MultiSelectWindow($config, "Mimetypes", "mimetypes");
  
    /* initialize required attributes */ 
    $this->parent           = &$parent;
    $this->ui               = get_userinfo();

    /* Set list strings */
    $this->SetTitle       (_("List of defined mime types"));
    $this->SetSummary     (_("List of defined mime types"));

    /* Result page will look like a headpage */
    $this->SetHeadpageMode();
    $this->EnableAplhabet(true);
  
    /* Disable buttonsm */
    $this->EnableCloseButton(false);
    $this->EnableSaveButton (false);

    /* set Page header */
    $action_col_size = 80;
    if($this->parent->snapshotEnabled()){
      $action_col_size += 38;
    }

    /* Toggle all selected / deselected */
    $chk = "<input type='checkbox' id='select_all' name='select_all' title='"._("Select all")."'
               onClick='toggle_all_(\"^item_selected_[0-9]*$\",\"select_all\");' >";

    /* set Page header */
    $this->AddHeader(array("string"=> $chk,          "attach"=>"style='width:20px;'"));
    $this->AddHeader(array("string" => "&nbsp;",                "attach" => "style='text-align:center;width:20px;'"));
    $this->AddHeader(array("string" => _("Mime type")." / "._("Department"), "attach" => "style=''"));
    $this->AddHeader(array("string" => _("Actions"),            "attach" => "style='width:".$action_col_size."px;border-right:0px;text-align:right;'"));

    /* Add SubSearch checkbox */
    $this->AddCheckBox("SubSearch",  msgPool::selectToView("","subsearch"), msgPool::selectToView("","subsearch_small"), false);

    /*                  Name                 ,Text                              ,Default  , Connect with alphabet  */
    $this->AddRegex   ("Regex",     _("Display mime types matching"),"*" , true);
  }


  function AddUserBoxToFilter($position)
  {
    $str = "";
    if(($position  == 2) && ($this->parent->IsReleaseManagementActivated($this->config))){
      $smarty = get_smarty();
      $releases = $this->parent->getReleases();
      $smarty->assign("mime_release" , $releases[$this->parent->mime_release]);
      $smarty->assign("mime_base"    , $releases[$this->parent->mime_base]);
      $smarty->assign("branchimage","plugins/goto/images/branch.png");
      $smarty->assign("mime_releases", $releases);
      $str = $smarty->fetch(get_template_path('release_select.tpl', TRUE));
    }
    return($str);
  }


  /* Create list header, with create / copy & paste etc*/
  function GenHeader()
  {
    /* Add default header */
    $listhead = MultiSelectWindow::get_default_header(false);
    if(!$this->parent->IsReleaseManagementActivated()){

      /* Get all departments within this subtree */
      $ui= get_userinfo();
      $first = "";
      $found = FALSE;
      $base = $this->config->current['BASE'];
      $options  = $this->create_department_list($this->module);

      /* And at least add a department selection box */
      $listhead .=  _("Base")."&nbsp;<select name='CurrentMainBase' onChange='mainform.submit()' class='center'>$options</select>".
        " <input class='center' type='image' src='images/lists/submit.png' align='middle' 
        title='"._("Submit department")."' name='submit_department' alt='"._("Submit")."'>&nbsp;";

    }

    /* Get acls */
    $ui       = get_userinfo();
    $acl      = $ui->get_permissions($this->parent->acl_base,"mimetypes/mimetype");
    $acl_all  = $ui->has_complete_category_acls($this->parent->acl_base,"mimetypes") ;


    /* If this is true we add an additional seperator. Just look a few lines below */  
    $add_sep = false;
 
    /* Create Layers menu */
    $s  = ".|"._("Actions")."|\n";

    /* Append create options */
    if(preg_match("/c/",$acl)) {
      $s .= "..|<img src='images/lists/new.png' alt='' border='0' class='center'>".
        "&nbsp;"._("Create")."|\n";
      $s.= "...|<input class='center' type='image' src='plugins/goto/images/list_new_mime.png' alt=''>".
        "&nbsp;"._("Mime type")."|mime_new|\n";
      $s.= "..|---|\n";
    }

    /* Multiple options */
    $s.= "..|<img src='images/lists/trash.png' alt='' border='0' class='center'>".
      "&nbsp;"._("Remove")."|"."remove_multiple|\n";

    /* Add Copy & Paste header */
    $s .= $this->parent->get_copypaste_header($this->selectedBase,$this->module);

    /* Add snapshot icons */
    $s .= $this->parent->get_snapshot_header($this->selectedBase,$this->module);

    $this->SetDropDownHeaderMenu($s);
    $this->SetListHeader($listhead);
  }


  /* Some basic settings */
  function execute()
  {
    $this->ClearElementsList();
    $this->GenHeader();
  }


  function setEntries($list)
  {
    /********************
      Variable init
     ********************/

    /* Create links */
    $editlink = "<a href='?plug=".$_GET['plug']."&amp;id=%s&amp;act=edit_entry'>%s</a>";
    $userimg  = "<img class='center' src='plugins/groups/images/groups.png' alt='User'    title='%s'>";
    $mimeimg  = "<img class='center' src='plugins/goto/images/select_mimetype.png' alt='A'  title='"._("Mime type")."'>";
    $empty    = "<img class='center' src='images/empty.png' style='width:16px;height:16px;' alt=''>";

    /* set Page header */
    $action_col_size = 80;
    if($this->parent->snapshotEnabled()){
      $action_col_size += 38;
    }

    /********************
      Attach objects
     ********************/

    foreach($list as $key => $val){

      /* Check FAI state 
       */  
      $FAIstate = $val['FAIstate'][0];

      $ui       = get_userinfo();
      $acl      = $ui->get_permissions($val['dn'],"mimetypes/mimetype");
      $acl_all  = $ui->has_complete_category_acls($val['dn'],"mimetypes") ;

      $action = "";

      /* Add copy & cut functionality */
      if(!preg_match("/freeze/i",$FAIstate)){
        $action.= $this->parent->get_copypaste_action($val['dn'],"mimetypes","mimetype");
      }else{
        $action.= "<img src='images/empty.png' class='center' alt=''>&nbsp;";
        $action.= "<img src='images/empty.png' class='center' alt=''>&nbsp;";
      }

      $action.= "<input class='center' type='image'
        src='images/lists/edit.png' alt='"._("edit")."' name='mime_edit_%KEY%' title='"._("Edit this entry")."'>";

      /* Add snapshot icon */
      if(!preg_match("/freeze/i",$FAIstate)){
        $action.= $this->parent->get_snapshot_action($val['dn'],$this->module);
      }else{
        $action.= "<img src='images/empty.png' class='center' alt=''>&nbsp;";
        $action.= "<img src='images/empty.png' class='center' alt=''>&nbsp;";
      }

      /* Add delete button */
      if(preg_match("/freeze/i",$FAIstate)){
        $action .= "<img src='plugins/goto/images/freeze.png' class='center' alt='!' title='"._("Freezed")."'>";
      }elseif(preg_match("/d/",$acl)){
        $action.= "<input class='center' type='image'
          src='images/lists/trash.png' alt='"._("delete")."' name='mime_del_%KEY%' title='"._("Delete this entry")."'>";
      }else{
        $action.= "<img src='images/empty.png' alt='&nbsp;' class='center'>";
      }

      $title = "title='".preg_replace('/ /', '&nbsp;', LDAP::fix($val['dn']))."'";

      if(!isset($val['description'][0])){
        $desc = "";
      }else{
        $desc = " - [ ".$val['description'][0]." ]";
      }

      /* Cutted objects should be displayed in light grey */
      $display = $val['cn'][0].$desc;
      if($this->parent->CopyPasteHandler){
        foreach($this->parent->CopyPasteHandler->queue as $queue_key => $queue_data){
          if($queue_data['dn'] == $val['dn']) {
            $display = "<font color='#999999'>".$display."</font>";
            break;
          }
        }
      }


      /* Create each field */
      $field0 = array("string" => "<input type='checkbox' id='item_selected_".$key."' name='item_selected_".$key."'>" ,
                      "attach" => "style='width:20px;'");
      $field1 = array("string" => sprintf($mimeimg,$val['dn']), "attach" => "style='text-align:center;width:20px;'");
      $field2 = array("string" => sprintf($editlink,$key,$display), "attach" => "style='' ".$title);
      $field3 = array("string" => preg_replace("/%KEY%/", $key, $action), "attach" => "style='width:".$action_col_size."px;border-right:0px;text-align:right;'");
      $this->AddElement(array($field0,$field1,$field2,$field3));
    }


    /* Create summary string for list footer */
    $num_deps=0;
    if(!$this->SubSearch){
      $num_deps = count($this->Added_Departments);
    }
    $num_objs = count($list);

    $num_obj_str = _("Number of listed mimetypes");
    $num_dep_str = _("Number of listed departments");

    $str = "<img class='center' src='plugins/goto/images/select_mimetype.png'
              title='".$num_obj_str."' alt='".$num_obj_str."'>&nbsp;".$num_objs."&nbsp;&nbsp;&nbsp;&nbsp;";
    $str.= "<img class='center' src='images/lists/folder.png'
              title='".$num_dep_str."' alt='".$num_dep_str."'>&nbsp;".$num_deps."&nbsp;&nbsp;&nbsp;&nbsp;";

    $this->set_List_Bottom_Info($str);
  }

  function Save()
  {
    MultiSelectWindow::Save();  
  }

  function save_object()
  {
    /* Save automatic created POSTs like regex, checkboxes */
    MultiSelectWindow::save_object(); 
  }

}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>