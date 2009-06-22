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

class passwordMethod
{
  var $config = false;
  var $attrs= array();
  var $display = FALSE;
  var $hash= "";
  var $lockable = TRUE;

  // Konstructor
  function passwordMethod($config)
  {
  }

  function create_template_hash($attrs)
  {
    if($this->get_hash_name() == ""){
      return("{crypt}N0T$3T4N0W");
    }else{
      return('{'.$this->get_hash().'}').'N0T$3T4N0W';
    }
  }

  function get_hash_name()
  {
  }


  function is_locked($config,$dn = "")
  {
    if(!$this->lockable) return FALSE;

    /* Get current password hash */
    $pwd ="";
    if(!empty($dn)){
      $ldap = $config->get_ldap_link();
      $ldap->cd($config->current['BASE']);
      $ldap->cat($dn);
      $attrs = $ldap->fetch();
      if(isset($attrs['userPassword'][0])){
        $pwd = $attrs['userPassword'][0];
      }
    }elseif(isset($this->attrs['userPassword'][0])){
      $pwd = $this->attrs['userPassword'][0];
    }
    return(preg_match("/^[^\}]*+\}!/",$pwd));
  }


  function lock_account($config,$dn = "")
  {
    if(!$this->lockable) return FALSE;

    /* Get current password hash */
    $pwd ="";
    $ldap = $config->get_ldap_link();
    $ldap->cd($config->current['BASE']);
    if(!empty($dn)){
      $ldap->cat($dn);
      $attrs = $ldap->fetch();
      if(isset($attrs['userPassword'][0])){
        $pwd = $attrs['userPassword'][0];
      }
    }elseif(isset($this->attrs['userPassword'][0])){
      $pwd = $this->attrs['userPassword'][0];
      $dn = $this->attrs['dn'];
    }

    /* We can only lock/unlock non-empty passwords */
    if(!empty($pwd)){

      /* Check if this entry is already locked. */
      if(preg_match("/^[^\}]*+\}!/",$pwd)){
        return(TRUE);
      }     
      
      /* Lock entry */
      $pwd = preg_replace("/(^[^\}]+\})(.*$)/","\\1!\\2",$pwd);
      $ldap->cd($dn);
      $ldap->modify(array("userPassword" => $pwd));
      return($ldap->success());
    }
    return(FALSE);
  }


  function unlock_account($config,$dn = "")
  {
    if(!$this->lockable) return FALSE;

    /* Get current password hash */
    $pwd ="";
    $ldap = $config->get_ldap_link();
    $ldap->cd($config->current['BASE']);
    if(!empty($dn)){
      $ldap->cat($dn);
      $attrs = $ldap->fetch();
      if(isset($attrs['userPassword'][0])){
        $pwd = $attrs['userPassword'][0];
      }
    }elseif(isset($this->attrs['userPassword'][0])){
      $pwd = $this->attrs['userPassword'][0];
      $dn = $this->attrs['dn'];
    }

    /* We can only lock/unlock non-empty passwords */
    if(!empty($pwd)){

      /* Check if this entry is already locked. */
      if(!preg_match("/^[^\}]*+\}!/",$pwd)){
        return (TRUE);
      }     
      
      /* Lock entry */
      $pwd = preg_replace("/(^[^\}]+\})!(.*$)/","\\1\\2",$pwd);
      $ldap->cd($dn);
      $ldap->modify(array("userPassword" => $pwd));
      return($ldap->success());
    }
    return(FALSE);
  }


  // this function returns all loaded classes for password encryption
  static function get_available_methods()
  {
    global $class_mapping, $config;
    $ret =false;
    $i =0;

    /* Only */
    if(!session::is_set("passwordMethod::get_available_methods")){
      foreach($class_mapping as $class => $path) {
        if(preg_match('/passwordMethod/i', $class) && !preg_match("/^passwordMethod$/i", $class)){
          $name = preg_replace ("/passwordMethod/i", "", $class);
          $test = new $class($config, "");
          if($test->is_available()) {
            $plugs= $test->get_hash_name();
            if (!is_array($plugs)){
              $plugs= array($plugs);
            }

            foreach ($plugs as $plugname){

              $cfg = $test->is_configurable();

              $ret['name'][$i]= $plugname;
              $ret['class'][$i]=$class;
              $ret['is_configurable'][$i]= $cfg;
              $ret['object'][$i]= $test;
              $ret['desc'][$i] = $test->get_description();
              $ret[$i]['name']  = $plugname;
              $ret[$i]['class'] = $class;
              $ret[$i]['object']= $test;
              $ret[$i]['is_configurable']= $cfg;
              $ret[$i]['desc'] = $test->get_description();
              $ret[$plugname]=$class;                    
              $i++;
            }
          }
        }
      }
      session::set("passwordMethod::get_available_methods",$ret);
    }
    return(session::get("passwordMethod::get_available_methods"));
  }
  

  function get_description()
  {
    return("");
  }


  // Method to let password backends remove additional information besides
  // the userPassword attribute
  function remove_from_parent()
  {
  }


  // Method to let passwords backends manage additional information
  // besides the userAttribute entry
  function set_password($password)
  {
    return(TRUE);
  }


  // Return true if this password method provides a configuration dialog
  function is_configurable()
  {
    return FALSE;
  }


  // Provide a subdialog to configure a password method
  function configure()
  {
    return "";
  }

  
  // Save information to LDAP
  function save($dn)
  {
  }


  // Try to find out if it's our hash...
  static function get_method($password_hash,$dn = "")
  {
    global $config;

    $methods= passwordMethod::get_available_methods();

    foreach ($methods['class'] as $class){

        $test = new $class($config,$dn);
#        All listed methods are available. 
#        if(!$test->is_available())continue;
        $method= $test->_extract_method($password_hash);
        if ($method != ""){
          $test->set_hash($method);
          return $test;
        }
    }

    msg_dialog::display(_("Error"), _("Cannot find a suitable password method for the current hash!"), ERROR_DIALOG);

    return NULL;
  }


  function _extract_method($password_hash)
  {
    $hash= $this->get_hash_name();
    if (preg_match("/^\{$hash\}/i", $password_hash)){
      return $hash;
    }

    return "";
  }


  static function make_hash($password, $hash)
  {
    global $config;

    $methods= passwordMethod::get_available_methods();
    $tmp= new $methods[$hash]($config);
    $tmp->set_hash($hash);
    return $tmp->generate_hash($password);
  }


  function set_hash($hash)
  {
    $this->hash= $hash;
  }


  function get_hash()
  {
    return $this->hash;
  }

  function adapt_from_template($dn)
  {
    return($this);
  }
}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>