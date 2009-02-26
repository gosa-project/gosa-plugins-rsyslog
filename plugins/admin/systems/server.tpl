<table summary="" width="100%">
 <tr>
  <td style="vertical-align:top; width:50%;">
	<table summary="">
	 <tr>
	  <td><LABEL for="cn">{t}Server name{/t}</LABEL>{$must}</td>
	  <td><input name="cn" id="cn" size=20 maxlength=60 value="{$cn}" {$cnACL}></td>
	 </tr>
	 <tr>
	  <td><LABEL for="description">{t}Description{/t}</LABEL></td>
	  <td><input name="description" id="description" size=25 maxlength=80 value="{$description}" {$descriptionACL}></td>
	 </tr>
 	 <tr>
	  <td><br><LABEL for="base">{t}Base{/t}</LABEL>{$must}</td>
	  <td>
	   <br>
	   <select size="1" name="base" id="base" title="{t}Choose subtree to place terminal in{/t}" {$baseACL}>
	    {html_options options=$bases selected=$base_select}
	   </select>
     	{if $baseACL == ""}
            <input type="image" name="chooseBase" src="images/folder.png" class="center" title="{t}Select a base{/t}">
        {else}
            <img src="images/folder_gray.png" class="center" title="{t}Select a base{/t}">
        {/if}
		</td>
	  </tr>
	</table>
  </td>
  <td  style="vertical-align:top;border-left:1px solid #A0A0A0;">
	<table summary="">
   	<tr>
     <td>{t}Mode{/t}</td>
     <td>
      <select name="gotoMode" title="{t}Select terminal mode{/t}" {$gotoModeACL}>
       {html_options options=$modes selected=$gotoMode}
      </select>
     </td>
    </tr>
	</table>
  </td>
 </tr>
</table>

<p class="plugbottom" style="margin-bottom:0px; padding:0px;">&nbsp;</p>

{$netconfig}

{if $fai_activated}
<p class="plugbottom" style="margin-bottom:0px; padding:0px;">&nbsp;</p>

<h2><img class="center" alt="" align="middle" src="images/rocket.png"> {t}Action{/t}</h2>
<table summary="">
 <tr>
  <td>
   <select size="1" name="saction" title="{t}Select action to execute for this server{/t}">
    <option disabled>&nbsp;</option>
    {html_options options=$actions}
   </select>
  </td>
  <td>
   <input type=submit name="action" value="{t}Execute{/t}">
  </td>
 </tr>
</table>
{/if}

<!-- Place cursor -->
<script language="JavaScript" type="text/javascript">
  <!-- // First input field on page
	focus_field('cn');
  -->
</script>