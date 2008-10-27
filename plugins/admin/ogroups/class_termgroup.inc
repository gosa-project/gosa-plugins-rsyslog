<?php

class termgroup extends plugin
{
  /* attribute list for save action */
  var $attributes= array("gotoMode","gotoSyslogServer", "gotoNtpServer");
  var $objectclasses= array("gotoWorkstationTemplate");

  var $members= array();

  var $gotoMode           = "locked";
  var $gotoSyslogServer   = "";
  var $gotoNtpServer      = array();
  var $gotoNtpServers     = array();
  var $modes              = array();
  var $inheritTimeServer  = true;
  var $acl                ;
  var $is_account		  = true; 
  var $orig_dn		      = "";
  var $didAction		  = FALSE;

  var $mapActions   = array("reboot"          => "localboot",
                            "instant_update"  => "softupdate",
                            "update"          => "scheduledupdate",
                            "reinstall"       => "install",
                            "rescan"          => "",
                            "memcheck"        => "memcheck",
                            "sysinfo"         => "sysinfo");

  var $CopyPasteVars = array("gotoNtpServers","modes","inheritTimeServer");

  function termgroup ($config, $dn= NULL, $parent= NULL)
  {
    /***************
      Some  initialisations
     ***************/

    plugin::plugin($config, $dn, $parent);

    $ldap= $config->get_ldap_link();

    $this->is_account = true;
    $ui               = get_userinfo();
    $acl              = get_permissions ($this->dn, $ui->subtreeACL);
    $this->acl        = get_module_permission($acl, "ogroup", $this->dn);

    $this->modes["active"]= _("Activated");
    $this->modes["locked"]= _("Locked");
    $this->modes["memcheck"]= _("Memory test");
    $this->modes["sysinfo"]= _("System analysis");

    $this->orig_dn =	$this->dn;

    /*************** 
      Get mac addresses from member objects  
     ***************/

    /* We're only interested in the terminal members here, evaluate
       these... */
    if(isset($this->attrs['member'])){
      for ($i= 0; $i<$this->attrs['member']['count']; $i++){
        $member= $this->attrs['member'][$i];
        $ldap->cat($member, array('objectClass', 'macAddress', 'cn'));
        if (preg_match("/success/i", $ldap->error)){
          $attrs = $ldap->fetch();
          if (in_array("gotoTerminal", $attrs['objectClass']) ||
              in_array("gotoWorkstation", $attrs['objectClass'])){
            if (isset($attrs['macAddress'])){
              $this->members[$attrs['cn'][0]]= $attrs['macAddress'][0];
            } else {
              $this->members[$attrs['cn'][0]]= "";
            }
          }
        }
      }
    }

    /*************** 
      Perpare NTP settings 
     ***************/

    /* Create used ntp server array */
    $this->gotoNtpServer= array();
    if(isset($this->attrs['gotoNtpServer'])){
      $this->inheritTimeServer = false;
      unset($this->attrs['gotoNtpServer']['count']);
      foreach($this->attrs['gotoNtpServer'] as $server){
        $this->gotoNtpServer[$server] = $server;
      }
    }

    /* Set inherit checkbox state */
    if(in_array("default",$this->gotoNtpServer)){
      $this->inheritTimeServer = true;
      $this->gotoNtpServer=array();
    }

    /* Create available ntp options */
    $this->gotoNtpServers = $this->config->data['SERVERS']['NTP'];
    foreach($this->gotoNtpServers as $key => $server){
      if($server == "default"){
        unset($this->gotoNtpServers[$key]);
      }
    }


  }

  function check()
  {
     /* Call common method to give check the hook */
    $message= plugin::check();

    if (chkacl($this->acl, "create") != ""){
      $message[]= _("You have no permissions to create a workstation on this 'Base'.");
    }

    /* Check for valid ntpServer selection */
    if((!$this->inheritTimeServer) && (!count($this->gotoNtpServer))){
      $message[]= _("There must be at least one NTP server selected.");
    }
    return($message);
  }

  function remove_from_parent()
  {
    /* Workstation startup is using gotoWorkstationTemplate too,
        if we remove this oc all other not manged attributes will cause errors */
    if(isset($this->attrs['gotoKernelParameters'])){
      $this->objectclasses = array();
    }

    /* Remove acc */
    plugin::remove_from_parent();
    $ldap = $this->config->get_ldap_link();
    $ldap->cd($this->orig_dn);
    $ldap->modify($this->attrs);
    $this->handle_post_events("remove");
  }


  function update_term_member_FAIstate($act)
  {
    /* Get required informations */
    $og    = $this->parent->by_object['ogroup'];  
    $allobs= $og->objcache;

    /* Get correct value for FAIstate */
    $action= $this->mapActions[$act];

    /* Get ldap connection */
    $ldap= $this->config->get_ldap_link();
    $ldap->cd ($this->config->current['BASE']);

    /* Foreach member of mthis ogroup  ... */
    foreach($og->member as $key ){
  
      /* check objectClasses and create attributes array */
      $attrs = array("FAIstate" => $action);  
      for($i = 0; $i < $allobs[$key]['objectClass']['count'] ; $i ++){
        $attrs['objectClass'][] = $allobs[$key]['objectClass'][$i];
      }
      if(($attrs['FAIstate'] != "") && (!in_array("FAIobject",$attrs['objectClass']))){
        $attrs['objectClass'][] = "FAIobject";
      }
      if($attrs['FAIstate'] == ""){
        $attrs['FAIstate'] = array();
      }

      /* If this objects is workstation,terminal or server upodate FAIstate */ 
      if(preg_match("/(w|t|s)/i",$allobs[$key]['type'])){
        $ldap->cd ($key);
        $ldap->modify($attrs);
        show_ldap_error($ldap->get_error(),sprintf(_("Setting action state (FAIstate) failed for object '%s', value was '%s'."),$key,$action));
        
        if(!preg_match("/success/i",$ldap->get_error())) {
          gosa_log("FAILED !! Updating FAIstate to '".$action."' : ".$key);
        }else{
          gosa_log("OK.  Updating FAIstate to '".$action."' : ".$key);
        }
      }
    }
  }


  function execute()
  {
    /* Call parent execute */
    plugin::execute();


    /*************** 
      Handle requested action
     ***************/

    /* Watch for events */
    if (isset($_POST['action'])){
      $macaddresses="";
      $names="";
      foreach ($this->members as $cn => $macAddress){
        $macaddresses.= "$macAddress ";
        $names.= "$cn ";
      }

      if (isset($_POST['action']) && $this->acl == "#all#"){

        /* Update members fai state */
        $this->update_term_member_FAIstate(trim($_POST['saction']));

        $cmd= search_config($this->config->data['TABS'], "workgeneric", "ACTIONCMD");
        if ($cmd == ""){
          print_red(_("No ACTIONCMD definition found in your gosa.conf"));
        } else {
          exec ($cmd." ".escapeshellarg($_POST['saction'])." ".$macaddresses, $dummy, $retval);
          if ($retval != 0){
            print_red(sprintf(_("Execution of '%s' failed!"), $cmd));
          } else {
            $this->didAction= TRUE;
          }
        }
      }
    }


    /*************** 
      Add remove NTP server
     ***************/

    /* Add new ntp Server to our list */
    if((isset($_POST['addNtpServer'])) && (isset($_POST['gotoNtpServers'])) && $this->acl == "#all#"){
      $this->gotoNtpServer[$_POST['gotoNtpServers']] = $_POST['gotoNtpServers'];
    }

    /* Delete selected NtpServer for list of used servers  */
    if((isset($_POST['delNtpServer'])) && (isset($_POST['gotoNtpServerSelected'])) && $this->acl == "#all#"){
      foreach($_POST['gotoNtpServerSelected'] as $name){
        unset($this->gotoNtpServer[$name]);
      }
    }


    /*************** 
      Prepare smarty 
     ***************/

    /* Set government mode */
    $smarty= get_smarty();

    foreach($this->attributes as $attr){
      $smarty->assign($attr,      $this->$attr);
      $smarty->assign($attr."ACL",chkacl($this->acl,$this->$attr));
    }

    /* Variables */
    foreach(array("gotoMode","gotoNtpServer") as $val){
      $smarty->assign($val."_select", $this->$val);
      $smarty->assign($val."ACL", chkacl($this->acl, $val));
    }

    /* Check if workstation is online */
    $smarty->assign("actions", array(
          "halt" =>             _("Switch off"), 
          "reboot" =>           _("Reboot"),
          "instant_update" =>   _("Instant update"),
          "update" =>           _("Scheduled update"),
          "reinstall" =>        _("Reinstall"),
          "rescan" =>           _("Rescan hardware")));

    $smarty->assign("inheritTimeServer",$this->inheritTimeServer);
    $smarty->assign("modes", $this->modes);
    $smarty->assign("actionACL", chkacl($this->acl, 'action'));
    $smarty->assign("gotoNtpServers",$this->gotoNtpServers); 
    $smarty->assign("syslogservers", $this->config->data['SERVERS']['SYSLOG']); 
    $smarty->assign("gotoSyslogServer_select", $this->gotoSyslogServer); 

    /* Show main page */
    return ($smarty->fetch (get_template_path('termgroup.tpl', TRUE)));
  }

  function save_object()
  {
    plugin::save_object();  
    /* Set inherit mode */
    if(isset($_POST['workgeneric_posted'])){
      if(isset($_POST["inheritTimeServer"])){
        $this->inheritTimeServer = true;
      }else{
        $this->inheritTimeServer = false;
      }
    }
  }

  /* Save to LDAP */
  function save()
  {
    plugin::save();

    /***************
      Prepare special vars 
     ***************/

    /* Unset some special vars ... */
    foreach (array("gotoSyslogServer") as $val){
      if ($this->attrs[$val] == "default"){
        $this->attrs[$val]= array();
      }
    }

    /* Update ntp server settings */
    if($this->inheritTimeServer){
      $this->attrs['gotoNtpServer'] = "default";
    }else{
      /* Set ntpServers */
      $this->attrs['gotoNtpServer'] = array();
      foreach($this->gotoNtpServer as $server){
        $this->attrs['gotoNtpServer'][] = $server;
      }
    }


    /***************
      Write to ldap 
     ***************/

    /* Write back to ldap */
    $ldap= $this->config->get_ldap_link();
    $ldap->cd($this->dn);
    $this->cleanup();
    $ldap->modify ($this->attrs);

    if(!$this->didAction){
      $this->handle_post_events("modify");
    }
    show_ldap_error($ldap->get_error(), _("Saving workstation failed"));
  }
}

// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>