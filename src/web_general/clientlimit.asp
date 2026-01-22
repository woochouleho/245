<%SendWebHeadStr(); %>
<title>Client Limit</title>
<SCRIPT>
var client_limit=0;
<% initClientLimit(); %>
function saveChanges()
{
	if (checkDigit(document.clientlimit.clientlimitnum.value) == 0) {		
		alert('invalid Maximum Device Num!');
		document.clientlimit.clientlimitnum.focus();
		return false;
	}
	document.forms[0].save.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);	
	return true;
}

function clickenable()
{
	with(document.forms[0]){
		if(clientlimitenable[0].checked == true)
			document.getElementById("show_limit_num").style.display="none";
		else
			document.getElementById("show_limit_num").style.display="";
	}
}

function on_init()
{
	with(document.forms[0]){
		if(client_limit == 0)
		{
			clientlimitenable[0].checked = true;
		}
		else
		{
			clientlimitenable[1].checked = true;
			clientlimitnum.value = client_limit;
		}
	}
	clickenable();
}
</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title">Client Limit Configuration</p>
	<p class="intro_content">This page is used to configure the capability of force how many device can access to Internet!</p>
</div>
<form action=/boaform/formClientLimit method=POST name="clientlimit">
<div class="data_common data_common_notitle">
	<table>
	  <tr>
	      <th width="30%">Client Limit Capability:</th>
		  <td>
	      	<input type="radio" value="0" name="clientlimitenable" onChange="clickenable()"><% multilang(LANG_DISABLED); %>&nbsp;&nbsp;
	     	<input type="radio" value="1" name="clientlimitenable" onChange="clickenable()"><% multilang(LANG_ENABLED); %>
	      </td>
	      
	  </tr>
	  <tr id="show_limit_num">
	      <th>Maximum Device:</th>
	      <td width="70%"><input type="text" name="clientlimitnum" size="15" maxlength="15" value=""></td>
	  </tr>
	</table>
</div>
<div class="btn_ctl">
      <input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges()">&nbsp;&nbsp;
      <input type="hidden" value="/clientlimit.asp" name="submit-url">
      <input type="hidden" name="postSecurityFlag" value="">
</div>
</form>
<script>
	on_init();
</script>
</body>
</html>
