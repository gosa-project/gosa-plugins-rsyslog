<?php
/*
   This code is part of GOsa (https://gosa.gonicus.de)
   Copyright (C) 2003  Cajus Pollmeier
   Copyright (C) 2005  Guillaume Delecourt
   Copyright (C) 2005  Vincent Seynhaeve
   Copyright (C) 2005  Benoit Mortier

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

require_once "../include/php_writeexcel/class.writeexcel_workbook.inc.php";
require_once "../include/php_writeexcel/class.writeexcel_worksheet.inc.php";

function dump_ldap ($mode= 0)
{
  global $config;
  $ldap= $config->get_ldap_link();
  error_reporting (E_ALL & ~E_NOTICE);

  $display = "";
  if($mode == 2){	// Single Entry Export !
    $d =  base64_decode($_GET['d']);
    $n =  base64_decode($_GET['n']);
    $dn=$d.$n;
    $date=date('dS \of F Y ');
    $fname = tempnam("/tmp", "demo.xls");
    $workbook=& new writeexcel_workbook($fname);

    $title_title=& $workbook->addformat(array(
          bold    => 1,
          color   => 'green',
          size    => 11,
          underline => 2,
          font    => 'Helvetica'
          ));

    $title_bold=& $workbook->addformat(array(
          bold    => 1,
          color   => 'black',
          size    => 10,
          font    => 'Helvetica'
          ));
   # Create a format for the phone numbers
   $f_phone =& $workbook->addformat();
   $f_phone->set_align('left');
   $f_phone->set_num_format('\0#');


    switch ($d){
      case "ou=people," : 
        $user= 				   $ldap->gen_xls($dn,"(objectClass=*)",array("uid","dateOfBirth","gender","givenName","preferredLanguage"));
      $intitul=array(_("Birthday").":", _("Sex").":", _("Surname")."/"._("Given Name").":",_("Language").":");

      //name of the xls file
      $name_section=_("Users");
	
      $worksheet=& $workbook->addworksheet(_("Users"));
      $worksheet->set_column('A:B', 51);

      $user_nbr=count($user);
      $worksheet->write('A1',_("User List of ").$n._(" on ").$date,$title_title);
      $r=3;
      for($i=1;$i<$user_nbr;$i++)
      {
        if($i>1)
          $worksheet->write('A'.$r++,"");
        $worksheet->write('A'.$r++,_("User ID").": ".$user[$i][0],$title_bold);

        for($j=1;$j<5;$j++)
        {
          $r++;
          $worksheet->write('A'.$r,$intitul[$j-1]);
          $user[$i][$j]=utf8_decode ($user[$i][$j]);
          $worksheet->write('B'.$r,$user[$i][$j]);
        }
        $worksheet->write('A'.$r++,"");
      }
      break;

      case "ou=groups,": $groups= $ldap->gen_xls($dn,"(objectClass=*)",array("cn","memberUid"),TRUE,1);
      $intitul=array(_("Members").":");

      //name of the xls file
      $name_section=_("Groups");

      $worksheet =& $workbook->addworksheet(_("Groups"));
      $worksheet->set_column('A:B', 51);

      //count number of groups
      $groups_nbr=count($groups);
      $worksheet->write('A1',_("Groups of ").$n._(" on ").$date,$title_title);
      $r=3;
      for($i=1;$i<$groups_nbr;$i++)
      {
        $worksheet->write('A'.$r++,_("User ID").": ".$groups[$i][0][0],$title_bold);
        for($j=1;$j<=2;$j++)
        {
          $r++;
          $worksheet->write('A'.$r,$intitul[$j-1]);
          for($k=0;$k<= $groups[$i][$j]['count'];$k++)
          {
            $worksheet->write('B'.$r,$groups[$i][$j][$k]);
            $r++;
          }
        }
      }
      break;

      case "ou=computers,": $computers= $ldap->gen_xls($dn,"(objectClass=*)",array("cn","description","uid"));
      $intitul=array(_("Description").":",_("User ID").":");
      $worksheet =& $workbook->addworksheet(_("Computers"));
      $worksheet->set_column('A:B', 32);
      //count number of computers
      $computers_nbr=count($computers);
      $r=1;
      for($i=1;$i<$computers_nbr;$i++)
      {
        if($i>1)
          $worksheet->write('A'.$r++,"");
        $worksheet->write('A'.$r++,_("Common Name").": ".$computers[$i][0],$title_bold);
        for($j=1;$j<3;$j++)
        {
          $r++;
          $worksheet->write('A'.$r,$intitul[$j-1]);
          $computers[$i][$j]=utf8_decode ($computers[$i][$j]);
          $worksheet->write('B'.$r,$computers[$i][$j]);
        }
        $worksheet->write('A'.$r++,"");
      }
      break;

      case "ou=servers,ou=systems,": $servers= $ldap->gen_xls($dn,"(objectClass=*)",array("cn"));
      $intitul=array(_("Server Name").":");

      //name of the xls file
      $name_section=_("Servers");

      $worksheet =& $workbook->addworksheet(_("Servers"));
      $worksheet->set_column('A:B', 51);

      //count number of servers
      $servers_nbr=count($servers);
      $worksheet->write('A1',_("Servers of ").$n._(" on ").$date,$title_title);
      $r=3;
      $worksheet->write('A'.$r++,_("Servers").": ",$title_bold);
      for($i=1;$i<$servers_nbr;$i++)
      {
        for($j=0;$j<1;$j++)
        {
          $r++;
          $worksheet->write('A'.$r,$intitul[$j]);
          $servers[$i][$j]=utf8_decode ($servers[$i][$j]);
          $worksheet->write('B'.$r,$servers[$i][$j]);
        }
      }
      break;

      case "dc=addressbook,": //data about addressbook
        $address= $ldap->gen_xls($dn,"(objectClass=*)",array("cn","displayName","facsimileTelephoneNumber","givenName","homePhone","homePostalAddress","initials","l","mail","mobile","o","ou","pager","telephoneNumber","postalAddress","postalCode","sn","st","title"));

      $intitul=array(_("Common name").":",_("Display Name").":",_("Fax").":",_("Name")."/"._("Given Name").":",_("Home phone").":",_("Home postal address").":",_("Initials").":",_("Location").":",_("Mail address").":",_("Mobile phone").":",_("City").":",_("Postal address").":",_("Pager").":",_("Phone number").":",_("Address").":",_("Postal code").":",_("Surname").":",_("State").":",_("Function").":");
      
      //name of the xls file
      $name_section=_("Adressbook");

      $worksheet =& $workbook->addworksheet(_("Servers"));
      $worksheet->set_column('A:B', 51);

      //count number of entries
      $address_nbr=count($address);
      $worksheet->write('A1',_("Adressbook of ").$n._(" on ").$date,$title_title);
      $r=3;
      for($i=1;$i<$address_nbr;$i++)
      {
        if($i>1)
          $worksheet->write('A'.$r++,"");
        $worksheet->write('A'.$r++,_("Common Name").": ".$address[$i][0],$title_bold);
        for($j=1;$j<19;$j++)
        {
          $r++;
          $worksheet->write('A'.$r,$intitul[$j]);
          $address[$i][$j]=utf8_decode ($address[$i][$j]);
          $worksheet->write('B'.$r,$address[$i][$j],$f_phone);
        }
        $worksheet->write('A'.$r++,"");
      }

      break;

      default: echo "error!!";
    }

    $workbook->close();


    // We'll be outputting a xls
    header('Content-type: application/x-msexcel');

    // It will be called demo.xls
    header('Content-Disposition: attachment; filename='.$name_section.".xls");

    // The source is in original.xls
    readfile($fname);
    unlink ($fname);
  }
  elseif($mode == 3){ // Full Export !
    $dn =  base64_decode($_GET['dn']);

    //data about users
    $user= $ldap->gen_xls("ou=people,".$dn,"(objectClass=*)",array("uid","dateOfBirth","gender","givenName","preferredLanguage"));
    $user_intitul=array(_("BirthDate").":",_("Sex").":",_("Surname")."/"._("Given Name").":",_("Language").":");
    //data about groups
    $groups= $ldap->gen_xls("ou=groups,".$dn,"(objectClass=*)",array("cn","memberUid"),TRUE,1);
    $groups_intitul=array(_("Members").":");
    //data about computers
    $computers= $ldap->gen_xls("ou=computers,".$dn,"(objectClass=*)",array("cn","description","uid"));
    $computers_intitul=array(_("Description").":",_("Uid").":");
    //data about servers
    $servers= $ldap->gen_xls("ou=servers,ou=systems,".$dn,"(objectClass=*)",array("cn"));
    $servers_intitul=array(_("Name").":");
    //data about addressbook
    $address= $ldap->gen_xls("dc=addressbook,".$dn,"(objectClass=*)",array("cn","displayName","facsimileTelephoneNumber","givenName","homePhone","homePostalAddress","initials","l","mail","mobile","o","ou","pager","telephoneNumber","postalAddress","postalCode","sn","st","title"));
    $address_intitul=array("cn",_("DisplayName").":",_("Fax").":",_("Surname")."/"._("Given Name").":",_("Phone Number").":",_("Postal Adress").":",_("Initials").":",_("City").":",_("Email address").":",_("mobile").":",_("Organization").":",_("Organizational Unit").":",_("Pager").":",_("Phone Number").":",_("Postal Address").":",_("Postal Code").":",_("Sn").":",_("st").":",_("Title").":");

    //name of the xls file
    $name_section=_("Full");
    $date=date('dS \of F Y ');
    $fname = tempnam("/tmp", "demo.xls");
    $workbook =& new writeexcel_workbook($fname);
    $worksheet =& $workbook->addworksheet(_("Users"));
    $worksheet2 =& $workbook->addworksheet(_("Groups"));
    $worksheet3 =& $workbook->addworksheet(_("Servers"));
    $worksheet4 =& $workbook->addworksheet(_("Computers"));
    $worksheet5 =& $workbook->addworksheet(_("Adressbook"));

    $worksheet->set_column('A:B', 51);
    $worksheet2->set_column('A:B', 51);
    $worksheet3->set_column('A:B', 51);
    $worksheet4->set_column('A:B', 51);
    $worksheet5->set_column('A:B', 51);

    $title_title=& $workbook->addformat(array(
          bold    => 1,
          color   => 'green',
          size    => 11,
          font    => 'Helvetica'
          ));

    $title_bold =& $workbook->addformat(array(
          bold    => 1,
          color   => 'black',
          size    => 10,
          font    => 'Helvetica'
          ));

   # Create a format for the phone numbers
   $f_phone =& $workbook->addformat();
   $f_phone->set_align('left');
   $f_phone->set_num_format('\0#');

    //count number of users
    $user_nbr=count($user);
    $worksheet->write('A1',_("User List of ").$dn._(" on ").$date,$title_title);
    $r=3;
    for($i=1;$i<$user_nbr;$i++)
    {
      if($i>1)
        $worksheet->write('A'.$r++,"");
      $worksheet->write('A'.$r++,_("User ID").": ".$user[$i][0],$title_bold);
      for($j=1;$j<5;$j++)
      {
        $r++;
        $worksheet->write('A'.$r,$user_intitul[$j-1]);
        $user[$i][$j]=utf8_decode ($user[$i][$j]);
        $worksheet->write('B'.$r,$user[$i][$j]);
      }
      $worksheet->write('A'.$r++,"");
    }

    //count number of groups
    $groups_nbr=count($groups);
    $worksheet2->write('A1',_("Groups of ").$dn._(" on ").$date,$title_title);
    $r=3;
    for($i=1;$i<$groups_nbr;$i++)
    {
      $worksheet2->write('A'.$r++,_("User ID").": ".$groups[$i][0][0],$title_bold);
      for($j=1;$j<=2;$j++)
      {
        $r++;
        $worksheet2->write('A'.$r,$group_intitul[$j-1]);
        for($k=0;$k<= $groups[$i][$j]['count'];$k++)
        {
          $worksheet2->write('B'.$r,$groups[$i][$j][$k]);
          $r++;
        }
      }
    }

    //count number of servers
    $servers_nbr=count($servers);
    $worksheet3->write('A1',_("Servers of ").$dn._(" on ").$date,$title_title);
    $r=3;
    $worksheet3->write('A'.$r++,_("Servers").": ",$title_bold);
    for($i=1;$i<$servers_nbr;$i++)
    {
      for($j=0;$j<1;$j++)
      {
        $r++;
        $worksheet3->write('A'.$r,$servers_intitul[$j]);
        $servers[$i][$j]=utf8_decode ($servers[$i][$j]);
        $worksheet3->write('B'.$r,$servers[$i][$j]);
      }
    }

    //count number of computers
    $computers_nbr=count($computers);
    $worksheet4->write('A1',_("Computers of ").$dn._(" on ").$date,$title_title);
    $r=3;
    for($i=1;$i<$computers_nbr;$i++)
    {
      if($i>1)
        $worksheet->write('A'.$r++,"");
      $worksheet4->write('A'.$r++,_("Common Name").": ".$computers[$i][0],$title_bold);
      for($j=1;$j<3;$j++)
      {
        $r++;
        $worksheet4->write('A'.$r,$computers_intitul[$j-1]);
        $computers[$i][$j]=utf8_decode ($computers[$i][$j]);
        $worksheet4->write('B'.$r,$computers[$i][$j]);
      }
      $worksheet4->write('A'.$r++,"");
    }

    //count number of entries
    $address_nbr=count($address);
    $worksheet5->write('A1',_("Adressbook of ").$dn._(" on ").$date,$title_title);

    $r=3;
    for($i=1;$i<$address_nbr;$i++)
    {
      if($i>1)
        $worksheet5->write('A'.$r++,"");
      $worksheet5->write('A'.$r++,_("Common Name").": ".$address[$i][0],$title_bold);
      for($j=1;$j<19;$j++)
      {
        $r++;
        $worksheet5->write('A'.$r,$address_intitul[$j]);
        $address[$i][$j]=utf8_decode ($address[$i][$j]);
        $worksheet5->write('B'.$r,$address[$i][$j],$f_phone);
      }
      $worksheet5->write('A'.$r++,"");
    }
    $workbook->close();


    // We'll be outputting a xls
    header('Content-type: application/x-msexcel');

    // It will be called demo.xls
    header('Content-Disposition: attachment; filename='.$name_section.".xls");

    readfile($fname);

    unlink ($fname);
  }
  elseif($mode == 4){ // IVBB LDIF Export
    $dn =  base64_decode($_GET['dn']);
    /*$display= $ldap->gen_ldif($dn,"(objectClass=ivbbEntry)",array(
      "GouvernmentOrganizationalUnit","houseIdentifier","vocation",
      "ivbbLastDeliveryCollective","gouvernmentOrganizationalPersonLocality",
      "gouvernmentOrganizationalUnitDescription","gouvernmentOrganizationalUnitSubjectArea",
      "functionalTitle","role","certificateSerialNumber","userCertificate","publicVisible",
      "telephoneNumber","seeAlso","description","title","x121Address","registeredAddress",
      "destinationIndicator","preferredDeliveryMethod","telexNumber","teletexTerminalIdentifier",
      "telephoneNumber","internationaliSDNNumber","facsimileTelephoneNumber","street",
      "postOfficeBox","postalCode","postalAddress","physicalDeliveryOfficeName","ou",
      "st","l","audio","businessCategory","carLicense","departmentNumber","displayName",
      "employeeNumber","employeeType","givenName","homePhone","homePostalAddress",
      "initials","jpegPhoto","labeledURI","mail","manager","mobile","o","pager","photo",
      "roomNumber","secretary","userCertificate","x500uniqueIdentifier","preferredLanguage",
      "userSMIMECertificate","userPKCS12"));*/

    echo $display;
  }
}


/* Basic setup, remove eventually registered sessions */
@require_once ("../include/php_setup.inc");
@require_once ("functions.inc");
error_reporting (E_ALL);
session_start ();

/* Logged in? Simple security check */
if (!isset($_SESSION['ui'])){
  gosa_log ("Error: getldif.php called without session");
  header ("Location: ../index.php");
  exit;
}
$ui= $_SESSION["ui"];
$config= $_SESSION['config'];

header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Last-Modified: ".gmdate("D, d M Y H:i:s")." GMT");
header("Cache-Control: no-cache");
header("Pragma: no-cache");
header("Cache-Control: post-check=0, pre-check=0");

header("Content-type: text/plain");

/* Check ACL's */
$acl= get_permissions ($config->current['BASE'], $ui->subtreeACL);
$acl= get_module_permission($acl, "all", $config->current['BASE']);
if (chkacl($acl, "all") != ""){
  header ("Location: ../index.php");
  exit;
}

switch ($_GET['ivbb']){
  case 2: dump_ldap (2);
          break;

  case 3: dump_ldap (3);
          break;

  case 4: dump_ldap (4);
          break;

  default:
          echo "Error in ivbb parameter. Request aborted.";
}
// vim:tabstop=2:expandtab:shiftwidth=2:filetype=php:syntax:ruler:
?>
