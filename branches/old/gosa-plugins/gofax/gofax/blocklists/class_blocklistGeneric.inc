<?php

define("BLOCK_LIST_RECEIVE" , 0);
define("BLOCK_LIST_SEND" , 1);

class blocklistGeneric extends plugin
{
  /* Definitions */
  var $plHeadline= "Fax blocklists";
  var $plDescription= "This does something";

  var $cn ="";
  var $description = "";
  var $base = "";  

  var $type;
  var $goFaxBlocklist = array();
  var $readonly = FALSE;
  var $view_logged = FALSE;
  var $attributes = array("cn","description");

  var $ignore_account = TRUE;

   var $orig_base = "";
   var $orig_dn = "";

  
  function __construct($config,$dn = "new")
  {
    plugin::plugin($config,$dn);

    /* Set default list type */
    $this->type = BLOCK_LIST_SEND;

    /* Load defined numbers */
    if($dn != "new"){
  
      /* We will not be able to rename edited blocklists */
      $this->readonly = TRUE;
  
      /* Get blocklist type and defined numbers */
      if (in_array("goFaxSBlock",$this->attrs['objectClass'])){
        if(isset($this->attrs["goFaxSBlocklist"])){
          for ($i= 0; $i<$this->attrs["goFaxSBlocklist"]["count"]; $i++){
            $this->goFaxBlocklist[]= $this->attrs["goFaxSBlocklist"][$i];
          }
        }
        $this->type= BLOCK_LIST_SEND;
      } elseif (in_array("goFaxRBlock",$this->attrs['objectClass'])){
        if(isset($this->attrs["goFaxRBlocklist"])){
          for ($i= 0; $i<$this->attrs["goFaxRBlocklist"]["count"]; $i++){
            $this->goFaxBlocklist[]= $this->attrs["goFaxRBlocklist"][$i];
          }
        }
        $this->type= BLOCK_LIST_RECEIVE;
      }
    }

    /* Set base */
    if ($this->dn == "new"){
      if(session::is_set('CurrentMainBase')){
        $this->base = session::get('CurrentMainBase');
      }else{
        $ui= get_userinfo();
        $this->base= dn2base($ui->dn);
      }
    } else {
      $this->base =preg_replace ("/^[^,]+,[^,]+,[^,]+,/","",$this->dn);
    }
    
    $this->orig_base = $this->base;
    $this->orig_dn   = $this->dn;
  }

  public function execute()
  {
    /* Log view */
    if(!$this->view_logged){
      $this->view_logged = TRUE;
      new log("view","blocklist/".get_class($this),$this->dn);
    }


    /**************
     * Base select dialog 
     **************/

    $once = true;
    foreach($_POST as $name => $value){
      if(preg_match("/^chooseBase/",$name) && $once){
        $once = false;
        $this->dialog = new baseSelectDialog($this->config,$this,$this->get_allowed_bases());
        $this->dialog->setCurrentBase($this->base);
      }
    }

    /* Dialog handling */
    if(is_object($this->dialog)){
      /* Must be called before save_object */
      $this->dialog->save_object();

      if($this->dialog->isClosed()){
        $this->dialog = false;
      }elseif($this->dialog->isSelected()){

        /* A new base was selected, check if it is a valid one */
        $tmp = $this->get_allowed_bases();
        if(isset($tmp[$this->dialog->isSelected()])){
          $this->base = $this->dialog->isSelected();
        }
        $this->dialog= false;

      }else{
        return($this->dialog->execute());
      }
    }


    /***************
      Add numer to blocklist
     ***************/


    /* Handle interactions: add */
    if (isset($_POST['add_number']) && $_POST['number'] != ""){
      if (tests::is_phone_nr($_POST['number']) || preg_match ("/^[\/0-9 ()\^\.\$+*-]+$/",$_POST['number'])){
        $this->addNumber ($_POST['number']);
      } else {
        msg_dialog::display(_("Error"), msgPool::invalid(_("Phone number")), ERROR_DIALOG);
      }
    }


    /***************
      Delete number from list
     ***************/

    /* Handle interactions: delete */
    if (isset($_POST['delete_number']) && isset($_POST['numbers']) && count($_POST['numbers']) > 0){
      $this->delNumber ($_POST['numbers']);
    }


    /***************
      Template output
     ***************/

    $smarty = get_smarty();
    foreach($this->attributes as $name){
      $smarty->assign($name,$this->$name);
    }
    $tmp = $this->plInfo();
    foreach($tmp['plProvidedAcls'] as $name => $translation){
      $smarty->assign($name."ACL",$this->getacl($name));
    }
    $smarty->assign("goFaxBlocklist",$this->goFaxBlocklist);
    $smarty->assign("cnACL",$this->getacl("cn",$this->readonly));
    $smarty->assign("typeACL",$this->getacl("type",$this->readonly));
    $smarty->assign("base",$this->base);
    $smarty->assign("bases", $this->get_allowed_bases());
    $smarty->assign("types", array(BLOCK_LIST_SEND => _("send"), BLOCK_LIST_RECEIVE => _("receive")));
    $smarty->assign("type", $this->type);
    $smarty->assign("dn", $this->dn);
    return($smarty->fetch(get_template_path('generic.tpl', TRUE)));
  }


  public function save_object()
  {
    if(isset($_POST['blocklist_posted'])){
      $tmp_cn = $this->cn;
      plugin::save_object();
      $this->cn = $tmp_cn;

      /* Save base, since this is no LDAP attribute */
      $tmp = $this->get_allowed_bases();
      if(isset($_POST['base'])){
        if(isset($tmp[$_POST['base']])){
          $this->base= $_POST['base'];
        }
      }
      $tmp = $this->attributes;
      $tmp[] = "type";
      foreach($tmp as $attr){
        if(in_array($attr,array("cn","type")) && $this->readonly){
          continue;
        }elseif(isset($_POST[$attr]) && $this->acl_is_writeable($attr)){
          $this->$attr = $_POST[$attr];
        }
      }
    }
  }


  function remove_from_parent()
  {
    $ldap= $this->config->get_ldap_link();
    $ldap->rmDir($this->dn);
    new log("remove","gofaxlist/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
    if (!$ldap->success()){
      msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, 0, get_class()));
    }
    $this->handle_post_events("remove");
  }


  /* Check values */
  function check()
  {
    /* Call common method to give check the hook */
    $message= plugin::check();

    /* check syntax: must cn */
    if ($this->cn == ""){
      $message[]= msgPool::required(_("Name"));
    } else {
      if (!tests::is_uid($this->cn)){
        $message[]= msgPool::invalid(_("Name"));
      }
      if ($this->dn == 'new'){
        $ldap= $this->config->get_ldap_link();
        $ldap->cd (get_ou('blocklistou').$this->config->current["BASE"]);
        $ldap->search ("(&(|(objectClass=goFaxSBlock)(objectClass=goFaxRBlock))(cn=".$this->cn."))", array("cn"));
        if ($ldap->count() != 0){
          $message[]= msgPool::duplicated(_("Name"));
        }
      }
    }
    
    /* Check if we are allowed to create or move this object
     */
    if($this->orig_dn == "new" && !$this->acl_is_createable($this->base)){
      $message[] = msgPool::permCreate();
    }elseif($this->orig_dn != "new" && $this->base != $this->orig_base && !$this->acl_is_moveable($this->base)){
      $message[] = msgPool::permMove();
    }
  
    return $message;
  }


  /* Save to LDAP */
  function save()
  {
    /* Type selection */
    if ($this->type == BLOCK_LIST_SEND){
      $type= "goFaxSBlocklist";
      $this->objectclasses = array("goFaxSBlock");
    } else {
      $type= "goFaxRBlocklist";
      $this->objectclasses = array("goFaxRBlock");
    }

    plugin::save();

    /* Add list */
    if (count($this->goFaxBlocklist)){
      $this->attrs[$type]= $this->goFaxBlocklist;
    }

    /* Write back to ldap */
    $ldap= $this->config->get_ldap_link();
    $ldap->cd($this->base);
    $ldap->cat($this->dn, array('dn'));
    if ($ldap->count()){
      if (!isset($this->attrs[$type])){
        $this->attrs[$type]= array();
      }
      $ldap->cd($this->dn);
      $this->cleanup();
      $ldap->modify($this->attrs);
      new log("modify","faxblocklist/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
      $this->handle_post_events("modify");
    } else {
      $ldap->cd($this->config->current['BASE']);
      $ldap->create_missing_trees(preg_replace('/^[^,]+,/', '', $this->dn));
      $ldap->cd($this->dn);
      $ldap->add($this->attrs);
      new log("create","gofaxlist/".get_class($this),$this->dn,array_keys($this->attrs),$ldap->get_error());
      $this->handle_post_events("add");
    }

    if (!$ldap->success()){
      msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, 0, get_class()));
    }
  }


  /* Add number */
  function addNumber($number)
  {
    if (!in_array($number, $this->goFaxBlocklist)){
      $this->goFaxBlocklist[]= $number;
      sort($this->goFaxBlocklist);
    }
  }


  /* Remove number from list */
  function delNumber($numbers)
  {
    $tmp= array();
    foreach ($this->goFaxBlocklist as $val){
      if (!in_array($val, $numbers)){
        $tmp[]= $val;
      }
    }
    $this->goFaxBlocklist= $tmp;
  }


  function getCopyDialog()
  { 
    $smarty = get_smarty();
    $smarty->assign("cn",$this->cn);
    $str = $smarty->fetch(get_template_path("paste_generic.tpl",TRUE,dirname(__FILE__)));
    $ret = array();
    $ret['string'] = $str;
    $ret['status'] = "";
    return($ret);
  }

  function saveCopyDialog()
  {
    if(isset($_POST['cn'])){
      $this->cn = get_post('cn');
    }
  }

  
  function PrepareForCopyPaste($source)
  {
    plugin::PrepareForCopyPaste($source);

    /* We will not be able to rename edited blocklists */
    $this->readonly = TRUE;

    /* Get blocklist type and defined numbers */
    if (in_array("goFaxSBlock",$source['objectClass'])){
      if(isset($source["goFaxSBlocklist"])){
        for ($i= 0; $i<$source["goFaxSBlocklist"]["count"]; $i++){
          $this->goFaxBlocklist[]= $source["goFaxSBlocklist"][$i];
        }
      }
      $this->type= BLOCK_LIST_SEND;
    } elseif (in_array("goFaxRBlock",$source['objectClass'])){
      if(isset($source["goFaxRBlocklist"])){
        for ($i= 0; $i<$source["goFaxRBlocklist"]["count"]; $i++){
          $this->goFaxBlocklist[]= $source["goFaxRBlocklist"][$i];
        }
      }
      $this->type= BLOCK_LIST_RECEIVE;
    }

    /* Set base */
    if ($this->dn == "new"){
      if(session::is_set('CurrentMainBase')){
        $this->base = session::get('CurrentMainBase');
      }else{
        $ui= get_userinfo();
        $this->base= dn2base($ui->dn);
      }
    } else {
      $this->base =preg_replace ("/^[^,]+,[^,]+,[^,]+,/","",$this->dn);
    }
  }


  /* Return plugin informations for acl handling */
  static function plInfo()
  {
    return (array(
          "plShortName"       => _("Fax"),
          "plDescription"     => _("Fax blocklists"),
          "plSelfModify"      => FALSE,
          "plDepends"         => array(),

          "plPriority"    => 0,
          "plSection"     => array("administration" => _("Fax blocklists")),
          "plCategory"    => array("gofaxlist"      => array("description" => _("Fax blocklists"),
              "objectClass" => array("goFaxRBlock","goFaxSBlock"))),
          "plProvidedAcls" => array(
            "cn"              => _("Name"),
            "description"     => _("Description"),
            "base"            => _("Base"),
            "goFaxBlocklist"  => _("Blocklist"),
            "type"            => _("Blocklist type"))
          ));
  }

}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>