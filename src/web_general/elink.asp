<% SendWebHeadStr();%>
<title>Elink <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
var elink_enable_mib=<% checkWrite("elink_enable"); %>
var elink_sync_mib=<% checkWrite("elink_sync"); %>
var elink_sdk_support=<% checkWrite("elink_cloud_sdk"); %>
function LoadSetting()
{
	if(elink_enable_mib)
	{
		document.elink.elink_enable[0].checked=true;
		document.elink.elink_enable[1].checked=false;
	}
	else
	{
		document.elink.elink_enable[0].checked=false;
		document.elink.elink_enable[1].checked=true;
		document.getElementById("sync_configure").style.display="none";
	}
	if(elink_sync_mib)
	{
		document.elink.elink_sync[0].checked=true;
		document.elink.elink_sync[1].checked=false;
	}
	else
	{
		document.elink.elink_sync[0].checked=false;
		document.elink.elink_sync[1].checked=true;
	}

	if(elink_sdk_support)
		document.getElementById("elinksdk").style.display="";
	else
		document.getElementById("elinksdk").style.display="none";

}
function elink_sync_func(value)
{
	if(value ==0) //disable sync config
	{
		document.elink.elink_sync[0].checked=false;
	}
	else //enable sync config
	{
		document.elink.elink_sync[1].checked=false;
	}

}
function elink_configure(value)
{
	if(value == 0) //disable elink
	{
		document.elink.elink_enable[0].checked=false;
		document.getElementById("sync_configure").style.display="none";
	}
	else	//enable elink
	{
		document.elink.elink_enable[0].checked=true;
		document.getElementById("sync_configure").style.display="";
	}

}
function saveChanges(obj)
{
	obj.isclick=1;
	postTableEncrypt(document.elink.postSecurityFlag, document.elink);
	return true;
}
function getFileType(str)
{
	if(str.value == "")
		return;
	var name = str.split(".");
	var type = name[name.length-1];
	return type;
}
function sendClicked(F)
{
  if(document.upload.binary.value == ""){
      	document.upload.binary.focus();
  	alert(upload_send_file_empty);
  	return false;
  }
  var fileType = getFileType(document.upload.binary.value);
  if(fileType != "tar"){      	
  	document.upload.binary.focus();
  	alert(upload_invalid_format);
  	return false;
  }
  F.submit();
}
</SCRIPT>
</head>

<body onload="LoadSetting()">
<blockquote>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_ELINK_HEADER); %></p>
	<p class="intro_content"><% multilang(LANG_ELINK_EXPLAIN); %></p>
</div>

<form action=/boaform/formElinkSetup method=POST name="elink">
      <input type="hidden" value="/elink.asp" name="submit-url">

<div class="data_common data_common_notitle">
<table border=0 width="500" cellspacing=4 cellpadding=0>

  <tr>
      <td width="30%"><font size=2><b>elink:</b></td>
      <td width="70%"><font size=2>
      <input type="radio" name="elink_enable" value="1" onClick="elink_configure(1)"><% multilang(LANG_ELINK_ENABLE); %>&nbsp;&nbsp;
      <input type="radio" name="elink_enable" value="0" onClick="elink_configure(0)"><% multilang(LANG_ELINK_DISABLE); %>
      </td>
  </tr>
  <tr id="sync_configure">
		<td width="30%"><font size=2><b><% multilang(LANG_ELINK_SYNC_CONFIG); %></b></td>
		<td width="70%"><font size=2>
		<input type="radio" name="elink_sync" id="sync_yes"value="1" onClick="elink_sync_func(1)"><% multilang(LANG_ELINK_SYNC_CONFIG_YES); %>&nbsp;&nbsp;
		<input type="radio" name="elink_sync" id="sync_no" value="0" onClick="elink_sync_func(0)"><% multilang(LANG_ELINK_SYNC_CONFIG_NO); %>
		</td>
	</tr>

</table>
</div>
  <div class="btn_ctl">
      <input type="submit" value="<% multilang(LANG_ELINK_SAVE); %>" name="save" onClick="return saveChanges(this)">&nbsp;&nbsp;
      <input type="reset" value="<% multilang(LANG_ELINK_RESET); %>" name="reset" onClick="resetClick()">
      <input type="hidden" name="postSecurityFlag" value="">
  </div>

</form>

<br/>
<br/>
<br/>

<form id="elinksdk" method="post" action="boaform/formElinkSDKUpload" enctype="multipart/form-data" name="upload">

<div class="intro_main ">
<p class="intro_title"><% multilang(LANG_ELINKSDK_HEADER); %></p>
</div>


<div class="data_common data_common_notitle">
<table border="0" cellspacing="4" width="500">
	<tr>
    	<td width="40%"><font size=2><b><% multilang(LANG_ELINKSDK_VERSION);%></b>&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td width="60%"><font size=2><%checkWrite("elinksdkVersion"); %></td>
	</tr>
  	<tr>
      	<td width="20%"><font size=2><b><% multilang(LANG_UPLOAD_FILE_SELECT);%></b>&nbsp;&nbsp;&nbsp;&nbsp;</td>
      	<td width="80%"><font size=2><input class="inner_btn" type="file" name="binary"  size=20></td>
  	</tr>
</table>
</div>

 <div class="btn_ctl">
<p> <input onclick=sendClicked(this.form) type=button value="<% multilang(LANG_ELINK_UPLOAD);%>" name="send">&nbsp;&nbsp;  
    <script>document.upload.send.value = upload_send;</script>

	<input type="hidden" value="/elink.asp" name="submit-url">

</p>
 </div>

</form>

</blockquote>
</body>

</html>

