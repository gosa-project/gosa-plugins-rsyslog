<?php
class applicationParameters extends plugin
{
  /* Parameters  */
  var $option_name= array();
  var $option_value= array();

  /* attribute list for save action */

  var $CopyPasteVars = array("option_name","option_value");
  var $attributes= array("gosaApplicationParameter");
  var $objectclasses= array();
  var $ui;

  function applicationParameters ($config, $dn= NULL, $parent= NULL)
  {
	plugin::plugin ($config, $dn, $parent);

	$this->gosaApplicationParameter = array();

	if (isset($this->attrs['gosaApplicationParameter'])){
		$this->is_account= TRUE;
		for ($i= 0; $i<$this->attrs['gosaApplicationParameter']['count']; $i++){
			$option= preg_replace('/^[^:]+:/', '',
					$this->attrs['gosaApplicationParameter'][$i]);
			$name= preg_replace('/:.*$/', '',
					$this->attrs['gosaApplicationParameter'][$i]);
			$this->option_name[$i]= $name;
			$this->option_value[$i]= $option;
		}
	} else {
		$this->is_account= FALSE;
	}
	$this->ui = get_userinfo();
  }

  function execute()
  {
	/* Call parent execute */
	plugin::execute();

	/* Do we need to flip is_account state? */
	if (isset($_POST['modify_state']) && chkacl($this->acl,"gotoLogonScript")==""){
		$this->is_account= !$this->is_account;
	}

	/* Show tab dialog headers */
	$display= "";
	if ($this->parent != NULL){
		if ($this->is_account){
			$display= $this->show_header(_("Remove options"),
				_("This application has options. You can disable them by clicking below."));
		} else {
			$display= $this->show_header(_("Create options"),
				_("This application has options disabled. You can enable them by clicking below."));
			$this->parent->by_object['application']->generateTemplate();
			return ($display);
		}
	}

	/* Add option to list */
	if (isset($_POST['add_option'])){
		$i= count($this->option_name);
		$this->option_name[$i]= "";
		$this->option_value[$i]= "";
	}

	if(chkacl($this->acl,"gotoLogonScript") == ""){

		/* Remove value from list */
		for ($i= 0; $i<count($this->option_name); $i++){
			if (isset($_POST["remove$i"])){
				$k= 0;
				$on= array();
				$ov= array();
				for ($j= 0; $j<count($this->option_name); $j++){
					if ($j != $i){
						$on[$k]= $this->option_name[$j];
						$ov[$k]= $this->option_value[$j];
						$k++;
					}
				}
				$this->option_name= $on;
				$this->option_value= $ov;
				break;
			}
		}
	}

	/* Generate list of attributes */
	if (count($this->option_name) == 0){
		$this->option_name[]= "";
		$this->option_value[]= "";
	}

	$mode= "";
	if (chkacl($this->acl, "create") != ""){
		$mode= "disabled";
	}

	$table= "<table summary=\"\"><tr><td>"._("Variable")."</td><td>"._("Default value")."</td><td></td></tr>";
	if (count ($this->option_name)){
		for ($i= 0; $i < count($this->option_name); $i++){
			$table.="<tr><td><input name=\"option$i\" size=25 maxlength=50 ".
				"value=\"".$this->option_name[$i]."\" $mode></td><td><input name=\"value$i\" ".
				"size=60 maxlength=250 value=\"".$this->option_value[$i]."\" $mode><br></td><td>".
				"<input type=\"submit\" name=\"remove$i\" value=\"".
				_("Remove")."\" $mode></td></tr>";
		}
	}
	$table.= "</table>";

	if ($mode == ""){
		$table.="<input type=\"submit\" name=\"add_option\" value=\""._("Add option")."\">";
	}

	/* Show main page */
	$smarty= get_smarty();
	$smarty->assign("table", $table);
	$display.= $smarty->fetch(get_template_path('parameters.tpl', TRUE));
	$this->parent->by_object['application']->generateTemplate();
	return ($display);
  }

  function remove_from_parent()
  {
	$ldap= $this->config->get_ldap_link();

	/* Zero attributes */
	$this->attrs= array();
	$this->attrs['gosaApplicationParameter']= array();

        $ldap->cd($this->dn);
        @DEBUG (DEBUG_LDAP, __LINE__, __FUNCTION__, __FILE__,
                $this->attributes, "Save");
	$this->cleanup();
	$ldap->modify ($this->attrs); 

	show_ldap_error($ldap->get_error(), _("Removing application parameters failed"));

	/* Optionally execute a command after we're done */
	$this->handle_post_events('remove');
  }


  /* Save data to object */
  function save_object()
  {
	  if(chkacl($this->acl,"gotoLogonScript") == ""){
		  if (isset($_POST['option0'])){
			  for ($i= 0; $i<count($this->option_name); $i++){
				  $this->option_name[$i]= $_POST["option$i"];
				  $this->option_value[$i]= "";
				  if ($_POST["value$i"] != ""){
					  $this->option_value[$i]= $_POST["value$i"];
				  }
			  }
		  }
	  }
  }


  /* Check values */
  function check()
  {
	/* Call common method to give check the hook */
	$message= plugin::check();

	/* Check for valid option names */
	for ($i= 0; $i<count($this->option_name); $i++){
		if (!preg_match ("/^[a-z0-9_]+$/i", $this->option_name[$i])){
			$message[]= sprintf(_("Value '%s' specified as option name is not valid."), 
					$this->option_name[$i]);
		}
	}
	
	return $message;
  }


  /* Save to LDAP */
  function save()
  {
  	/* Generate values */
	$this->attrs= array();
	if (count($this->option_name) == 0){
		$this->attrs['gosaApplicationParameter']= array();
	} else {
		for ($i= 0; $i<count($this->option_name); $i++){
			$this->attrs['gosaApplicationParameter'][]= $this->option_name[$i].
				":".$this->option_value[$i];
		}
	}

	/* Write back to ldap */
	$ldap= $this->config->get_ldap_link();
	$ldap->cd($this->dn);
        @DEBUG (DEBUG_LDAP, __LINE__, __FUNCTION__, __FILE__,
                $this->attributes, "Save");
	$this->cleanup();
	$ldap->modify ($this->attrs); 

	show_ldap_error($ldap->get_error(), _("Saving applications parameters failed"));

	/* Optionally execute a command after we're done */
	$this->handle_post_events('modify');
  }

}

?>