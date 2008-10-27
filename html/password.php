<?php
/*
   This code is part of GOsa (https://gosa.gonicus.de)
   Copyright (C) 2003-2007  Cajus Pollmeier

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

function displayPWchanger()
{
  global $smarty;

  $smarty->display(get_template_path('password.tpl'));
  exit();
}

/* Load required includes */
require_once ("../include/php_setup.inc");
require_once ("functions.inc");
header("Content-type: text/html; charset=UTF-8");

session_start();

/* Destroy old session if exists.
    Else you will get your old session back, if you not logged out correctly. */
if(is_array($_SESSION) && count($_SESSION)){
  session_destroy();
  session_start();
}

/* Reset errors */
$_SESSION['js']                 = true;
$_SESSION['errors']             = "";
$_SESSION['errorsAlreadyPosted']= array();
$_SESSION['LastError']          = "";

/* Check if CONFIG_FILE is accessible */
if (!is_readable(CONFIG_DIR."/".CONFIG_FILE)){
  echo sprintf(_("GOsa configuration %s/%s is not readable. Aborted."), CONFIG_DIR,CONFIG_FILE);
  exit();
}

/* Parse configuration file */
$config= new config(CONFIG_DIR."/".CONFIG_FILE, $BASE_DIR);
$_SESSION['DEBUGLEVEL']= $config->data['MAIN']['DEBUGLEVEL'];
if ($_SERVER["REQUEST_METHOD"] != "POST"){
  @DEBUG (DEBUG_CONFIG, __LINE__, __FUNCTION__, __FILE__, $config->data, "config");
}

/* Set template compile directory */
if (isset ($config->data['MAIN']['COMPILE'])){
  $smarty->compile_dir= $config->data['MAIN']['COMPILE'];
} else {
  $smarty->compile_dir= '/var/spool/gosa';
}

/* Check for compile directory */
if (!(is_dir($smarty->compile_dir) && is_writable($smarty->compile_dir))){
  echo sprintf(_("Directory '%s' specified as compile directory is not accessible!"),
        $smarty->compile_dir);
  exit();
}

/* Check for old files in compile directory */
clean_smarty_compile_dir($smarty->compile_dir);

/* Language setup */
if ($config->data['MAIN']['LANG'] == ""){
  $lang= get_browser_language();
} else {
  $lang= $config->data['MAIN']['LANG'];
}
$lang.=".UTF-8";
putenv("LANGUAGE=");
putenv("LANG=$lang");
setlocale(LC_ALL, $lang);
$GLOBALS['t_language']= $lang;
$GLOBALS['t_gettext_message_dir'] = $BASE_DIR.'/locale/';

/* Set the text domain as 'messages' */
$domain = 'messages';
bindtextdomain($domain, "$BASE_DIR/locale");
textdomain($domain);

/* Generate server list */
$servers= array();
if (isset($_POST['server'])){
	$directory= validate($_POST['server']);
} else {
	$directory= $config->data['MAIN']['DEFAULT'];
}
foreach ($config->data['LOCATIONS'] as $key => $ignored){
	$servers[$key]= $key;
}
if (isset($_GET['directory']) && isset($servers[$_GET['directory']])){
	$smarty->assign ("show_directory_chooser", false);
	$directory= validate($_GET['directory']);
} else {
	$smarty->assign ("server_options", $servers);
	$smarty->assign ("server_id", $directory);
	$smarty->assign ("show_directory_chooser", true);
}

/* Set config to selected one */
$config->set_current($directory);
$_SESSION['config']= $config;

if ($_SERVER["REQUEST_METHOD"] != "POST"){
  @DEBUG (DEBUG_TRACE, __LINE__, __FUNCTION__, __FILE__, $lang, "Setting language to");
}


/* Check for SSL connection */
$ssl= "";
if (!isset($_SERVER['HTTPS']) ||
    !stristr($_SERVER['HTTPS'], "on")) {

  if (empty($_SERVER['REQUEST_URI'])) {
    $ssl= "https://".$_SERVER['HTTP_HOST'].
      $_SERVER['PATH_INFO'];
  } else {
    $ssl= "https://".$_SERVER['HTTP_HOST'].
      $_SERVER['REQUEST_URI'];
  }
}

/* If SSL is forced, just forward to the SSL enabled site */
if ($config->data['MAIN']['FORCESSL'] == 'true' && $ssl != ''){
  header ("Location: $ssl");
  exit;
}

/* Check for selected password method */
$method= $config->current['HASH'];
if (isset($_GET['method'])){
	$method= validate($_GET['method']);
	$tmp = new passwordMethod($config);
	$available = $tmp->get_available_methods_if_not_loaded();
	if (!isset($available[$method])){
		echo _("Error: Password method not available!");
		exit;
	}
}


/* Check for selected user... */
if (isset($_GET['uid']) && $_GET['uid'] != ""){
	$uid= validate($_GET['uid']);
	$smarty->assign('display_username', false);
} elseif (isset($_POST['uid'])){
	$uid= validate($_POST['uid']);
	$smarty->assign('display_username', true);
} else {
	$uid= "";
	$smarty->assign('display_username', true);
}
$current_password= "";
$smarty->assign("changed", false);

/* Got a formular answer, validate and try to log in */
if ($_SERVER["REQUEST_METHOD"] == "POST" && isset($_POST['apply'])){

  /* Destroy old sessions, they cause a successfull login to relog again ...*/
  if(isset($_SESSION['_LAST_PAGE_REQUEST'])){
    $_SESSION['_LAST_PAGE_REQUEST'] = time();
  }

  $message= array();
  $current_password= $_POST['current_password'];

  /* Do new and repeated password fields match? */
  $new_password= $_POST['new_password'];
  if ($_POST['new_password'] != $_POST['new_password_repeated']){
	  $message[]= _("The passwords you've entered as 'New password' and 'Repeated new password' do not match.");
  } else {
	  if ($_POST['new_password'] == ""){
		  $message[]= _("The password you've entered as 'New password' is empty.");
	  }
  }

  /* Password policy fulfilled? */
  if (isset($config->data['MAIN']['PWDIFFER'])){
	  $l= $config->data['MAIN']['PWDIFFER'];
	  if (substr($_POST['current_password'], 0, $l) == substr($_POST['new_password'], 0, $l)){
		  $message[]= _("The password used as new and current are too similar.");
	  }
  }
  if (isset($config->data['MAIN']['PWMINLEN'])){
	  if (strlen($_POST['new_password']) < $config->data['MAIN']['PWMINLEN']){
		  $message[]= _("The password used as new is to short.");
	  }
  }

  /* Validate */
  if (!ereg("^[A-Za-z0-9_.-]+$", $uid)){
	  $message[]= _("Please specify a valid username!");
  } elseif (mb_strlen($_POST["current_password"], 'UTF-8') == 0){
	  $message[]= _("Please specify your password!");
  } else {

    /* Do we have the selected user somewhere? */
    $ui= ldap_login_user ($uid, $current_password);
    if ($ui == NULL){
      $message[]= _("Please check the username/password combination.");
    } else {
      /* Check acl... */
      $ca= get_permissions ($ui->dn, $ui->subtreeACL);
      $ca= get_module_permission($ca, "user", $ui->dn);
      if (chkacl($ca, "password") != ""){
        $message[]= _("You have no permissions to change your password.");
      }
    }
  }

  /* Do we need to show error messages? */
  if (count ($message) != 0){
	  /* Show error message and continue editing */
	  show_errors($message);
  } else {

	  /* Passed quality check, just try to change the password now */
	  $output= "";
	  if (isset($config->data['MAIN']['EXTERNALPWDHOOK'])){
		  exec($config->data['MAIN']['EXTERNALPWDHOOK']." ".$ui->username." ".
				  $_POST['current_password']." ".$_POST['new_password'], $resarr);
		  if(count($resarr) > 0) {
			  $output= join('\n', $resarr);
		  }
	  }
	  if ($output != ""){
		  $message[]= _("External password changer reported a problem: ".$output);
		  show_errors($message);
	  } else {
		  if ($method != ""){
			  change_password ($ui->dn, $_POST['new_password'], 0, $method);
		  } else {
			  change_password ($ui->dn, $_POST['new_password']);
		  }
		  gosa_log ("User/password has been changed");
		  $smarty->assign("changed", true);
	  }
  }


}

/* Parameter fill up */
$params= "";
foreach (array('uid', 'method', 'directory') as $index){
	$params.= "&amp;$index=".urlencode($$index);
}
$params= preg_replace('/^&amp;/', '?', $params);
$smarty->assign('params', $params);

/* Fill template with required values */
$smarty->assign ('date', gmdate("D, d M Y H:i:s"));
$smarty->assign ('uid', $uid);
$smarty->assign ('password_img', get_template_path('images/password.png'));

/* Displasy SSL mode warning? */
if ($ssl != "" && $config->data['MAIN']['WARNSSL'] == 'true'){
  $smarty->assign ("ssl", "<b>"._("Warning").":</b> "._("Session will not be encrypted.")." <a style=\"color:red;\" href=\"".htmlentities($ssl)."\"><b>"._("Enter SSL session")."</b></a>!");
} else {
  $smarty->assign ("ssl", "");
}

/* show login screen */
$smarty->assign ("PHPSESSID", session_id());
if (isset($_SESSION['errors'])){
  $smarty->assign("errors", $_SESSION['errors']);
}
if ($error_collector != ""){
  $smarty->assign("php_errors", $error_collector."</div>");
} else {
  $smarty->assign("php_errors", "");
}

displayPWchanger();

?>

</body>
</html>
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler: