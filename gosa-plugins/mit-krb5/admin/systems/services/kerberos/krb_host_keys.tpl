<p class="seperator">&nbsp;</p>
<h2><img class="center" alt="" src="images/lists/locked.png" align="middle">	{t}Host key{/t}</h2>
<table>
	<tr>
		<td>{t}Realm{/t}</td>
		<td>
			<select name="goKrbRealm" title="{t}Select a realm{/t}">
			{html_options options=$Realms selected=$goKrbRealm}
			</select>
		</td>
		<td><input type='submit' name="host_key_generate" value="{t}Generate{/t}"></td>
	</tr>
</table>
