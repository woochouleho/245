<% SendWebHeadStr();%>
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<title><% multilang(LANG_SAMBA); %> <% multilang(LANG_USER_ACCOUNT); %> <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
function postSamba( username, password, path, writable, select )
{
	return;
}

function changeListMenu()
{
	//var i=0;
	//var lstmenu = parent.document.getElementById("lstmenu").rows[0].cells[0];
	//lstmenu.innerHTML = "<br><p>&nbsp;&nbsp;<a id=\"thirdmenufont"+i+"\" onClick=\"on_thirdmenu(" + i + ")\"; style=\"color:#fff45c\" href=\"" + "app_samba_account.asp" + "\", target=\"contentIframe\">" + "USB存储共享" + "</a></p>";
	window.location.href="app_samba_account_add.asp";	
}

</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_SAMBA); %> <% multilang(LANG_USER_ACCOUNT); %></p>	
</div>

<form action=/boaform/formSambaAccount method=POST name="samba">

<div class="data_common data_common_notitle">
<table border=1 width="600" cellspacing=4 cellpadding=0>
  <% showSambaAccount(); %>
</table>
</div>
  <br>
<div class="btn_ctl clearfix">
  <tr>
   <td>
		<input class="link_bg" type="button" value="<% multilang(LANG_ADD); %>" name="gotoaddSamba" onclick="changeListMenu();">&nbsp;&nbsp; 
		<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE); %>" name="delSamba" onClick="return deleteClick(this)">
		<input type="hidden" value="/app_samba_account.asp" name="submit-url">
		&nbsp;&nbsp;
		<input type="hidden" name="postSecurityFlag" value="">
   </td>
  </tr>
</div>

</form>
</body>
</html>