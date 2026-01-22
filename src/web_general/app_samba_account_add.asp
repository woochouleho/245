<% SendWebHeadStr();%>
<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
<title><% multilang(LANG_SAMBA); %> <% multilang(LANG_USER_ACCOUNT); %> <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
var rcs = new Array();
with(rcs){<% listUsbDevices(); %>}
/********************************************************************
**          on document load
********************************************************************/
function addOptionToSelect(sel, txt, val) {
    var opt = document.createElement('option');
    opt.appendChild( document.createTextNode(txt) );
    
    if ( typeof val === 'string' ) {
        opt.value = val;
    }
    
    sel.appendChild(opt);
    return;
}

function removeAllOptions(sel) {
    var len, par;
    
    len = sel.options.length;
    for (var i=len; i; i--) {
        par = sel.options[i-1].parentNode;
        par.removeChild( sel.options[i-1] );
    }
}

function on_init()
{
#ifndef CONFIG_SMBD_SHARE_USB_ONLY
	var sel = document.getElementById('saveDir');
	removeAllOptions(sel);
	
	for(var i = 0; i < rcs.length; i++)
	{
		addOptionToSelect(sel, rcs[i].path, rcs[i].path);
	}
	addOptionToSelect(sel, "<% multilang(LANG_DEFAULT); %>", "<% multilang(LANG_DEFAULT); %>");
#endif
/*
	while(document.samba.saveDir.options.length >= 1)
		{document.samba.saveDir.options.remove(0);}

	for(var i = 0; i < rcs.length; i++)
	{
		document.samba.saveDir.options.add(new Option(rcs[i].path, rcs[i].path));
	}
	document.samba.saveDir.options.add(new Option("预设", ""));
	*/
}
function postSamba( username, password, path, writable, select )
{
	document.samba.username.value=username;
	document.samba.password.value=password;
#ifndef CONFIG_SMBD_SHARE_USB_ONLY
	document.samba.saveDir.value=path;
#endif
	document.samba.writable.value=writable;
	document.samba.select_id.value=select;	
}

function addClick(obj)
{
	if (document.samba.username.value=="") {
		alert("<% multilang(LANG_STRUSERNAMEEMPTY); %>");
		document.samba.username.focus();
		return false;
	}
	
	if (document.samba.password.value=="" ) {
		alert("<% multilang(LANG_STRPASSEMPTY); %>");
		document.samba.password.focus();
		return false;
	}
	
	obj.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function backMain()
{
	window.location.href='/app_samba_account.asp';
}

function checkChange(cb)
{
	if(cb.checked==true){
		cb.value = 1;
	}
	else{
		cb.value = 0;
	}
}	
</SCRIPT>
</head>

<body onLoad="on_init()">

<form id="form" action=/boaform/formSambaAccount method=POST name="samba">

<div class="data_common data_common_notitle">
<table border=0 width="600" cellspacing=4 cellpadding=0>
  <tr>
      <td width="30%"><% multilang(LANG_USERNAME); %>:</td>
      <td width="70%"><input type="text" name="username" size="30" maxlength="30"></td>
  </tr>
  <tr>
      <td width="30%"><% multilang(LANG_PASSWORD); %>:</td>
      <td width="70%"><input type="text" name="password" size="30" maxlength="30"></td>
  </tr>
#ifndef CONFIG_SMBD_SHARE_USB_ONLY
  <tr>
      <td width="30%"><% multilang(LANG_PATH); %>:</td>
      <td width="70%"><select size="1" name="saveDir" id="saveDir"></select></td>
  </tr>
#endif
  <tr>
	  <td width="70%"><% multilang(LANG_WRITABLE); %></td>
	  <td width="30%"><input type="checkbox" name="writeable" value=0 onChange="checkChange(this)"></td>
  </tr>
  <input type="hidden" value="" name="select_id">
</table>
</div> 
<br>
<div lass="btn_ctl">
  <tr>
		<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY); %>" name="addSamba" onClick="return addClick(this)">&nbsp;&nbsp;
		<input class="link_bg" type="button" value="<% multilang(LANG_UNDO); %>" name="delSamba" onclick="backMain()">&nbsp;&nbsp;
		<input type="hidden" value="/app_samba_account.asp" name="submit-url">
		<input type="hidden" name="postSecurityFlag" value="">
  </tr>
</div> 
</form>
</body>
</html>
