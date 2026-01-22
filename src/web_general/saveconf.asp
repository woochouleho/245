<% SendWebHeadStr();%>
<title><% multilang(LANG_BACKUP_AND_RESTORE_SETTINGS); %></title>

<script>
function resetClick(obj)
{
	if(confirm("<% multilang(LANG_DO_YOU_REALLY_WANT_TO_RESET_THE_CURRENT_SETTINGS_TO_FACTORY_DEFAULT); %>"))
	{
		obj.isclick = 1;
		postTableEncrypt(document.resetConfig.postSecurityFlag, document.resetConfig);
		return true;
	}
	else
		return false;
}

</script>

</head>
<body>
<div class="intro_main ">
    <p class="intro_title"><% multilang(LANG_RESET_SETTINGS_TO_DEFAULT); %></p>
    <p class="intro_content"> 현재 설정을 공장 기본값으로 재설정할 수 있습니다.</p>
</div>
  
<form action=/boaform/admin/formSaveConfig method=POST name="resetConfig">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th width="40%"><% multilang(LANG_RESET_SETTINGS_TO_DEFAULT); %>:</th>
			<td width="60%">
				<input class="inner_btn" type="submit" value="<% multilang(LANG_RESET); %>" name="reset" onclick="return resetClick(this)">
				<input class="inner_btn" type="hidden" value="/saveconf.asp" name="submit-url">
			</td>
			<input type="hidden" name="postSecurityFlag" value="">
		</tr>
	</table>
</div>
</form>
</body>
</html>
