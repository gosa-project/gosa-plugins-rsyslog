<?php

class apptabs extends tabs
{
  var $parent   = FALSE;
  var $FAIstate = "";

  function apptabs($config, $data, $dn,$category)
  {
    tabs::tabs($config, $data, $dn,$category);

    /* Add references/acls/snapshots */
    #$this->addSpecialTabs();
  }

  function set_FAIstate($state)
  {
    $this->FAIstate = $state;
    foreach($this->by_name as $name => $desc){
      $this->by_object[$name]->FAIstate = $state;
    }
  }

  function save($ignore_account= FALSE)
  {
    $baseobject= $this->by_object['application'];

    /* Check for new 'dn', in order to propagate the
       'dn' to all plugins */
    if($this->parent->IsReleaseManagementActivated()){
      $new_dn= "cn=".$baseobject->cn.",".$this->parent->app_release;
    }else{
      $new_dn= "cn=".$baseobject->cn.",".get_ou('applicationRDN').$baseobject->base;
    }

    /* Move group? */
    if ($this->dn != $new_dn){

      /* Write entry on new 'dn' */
      if ($this->dn != "new"){
        $baseobject->move($this->dn, $new_dn);
        $this->by_object['application']= $baseobject;
      }

      /* Happen to use the new one */
      $this->dn= $new_dn;
    }

    tabs::save();
  }

}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>