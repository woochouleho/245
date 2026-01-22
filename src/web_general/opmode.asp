<% SendWebHeadStr();%>
<title><% multilang(LANG_INTERNET_OP_MODE); %></title>
<SCRIPT>
var isRepeaterSupport = <% checkWrite("isRepeaterSupport"); %>;
var opmode = <% checkWrite("Internet OP Mode"); %>;

var time=25;
function countDown_start(){	
	time--;
	if(time >= 0){
		document.cntdown_frm.cntdwn_time.value = time;
		document.cntdown_frm.cntdwn_text.value = "<% multilang(LANG_PLEASE_WAIT_FOR_CONFIGURING_OPMODE); %>";
		setTimeout("countDown_start()", 1000);
	}	
	if(time == 0){
		window.location.reload();
	}
}

function countDown(){
 	document.getElementById('opmode_div').style.display = 'none';
	document.getElementById('cntdown_div').style.display = 'block';
	countDown_start();
}

function saveChanges()
{
	if(document.Internetmode.opmode.value == opmode)
		return false;
		
	//document.forms[0].save.isclick = 1;
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);	
	document.Internetmode.submit();	
	
	countDown();	
	return true;
}

function enabledgateway()
{
	document.Internetmode.opmode[0].checked = true;
	document.Internetmode.opmode.value=0;
	// OP_MODE=0,gateway=enabled
}

function enabledbridge()
{
	document.Internetmode.opmode[1].checked = true;
	document.Internetmode.opmode.value=1;
	//OP_MODE=1,bridge=enabled
}

function enabledrepeater()
{
	document.Internetmode.opmode[2].checked = true;
	document.Internetmode.opmode.value=3;
	//OP_MODE=1,repeater=enbaled
}

function on_init()
{	
	if(opmode == 0){
		document.Internetmode.opmode[0].checked = true;
		document.Internetmode.opmode[1].checked = false;
		if(isRepeaterSupport==1)
			document.Internetmode.opmode[2].checked = false;
	}else if(opmode ==1){
		document.Internetmode.opmode[0].checked = false;
		document.Internetmode.opmode[1].checked = true;
		if(isRepeaterSupport==1)
			document.Internetmode.opmode[2].checked = false;
	}else if(isRepeaterSupport==1 && opmode ==3){
		document.Internetmode.opmode[0].checked = false;
		document.Internetmode.opmode[1].checked = false;
		document.Internetmode.opmode[2].checked = true;
	}
	
	return true;
} 
</SCRIPT>
</head>

<BODY onLoad="on_init();">
	<div class="intro_main ">
		<p class="intro_title"><% multilang(LANG_OPERATION_MODE); %></p>
		<p class="intro_content"><% multilang(LANG_PAGE_DESC_CONFIGURE_INTERNET_OP_MODE_SETTING); %></p>
	</div>
<div id="opmode_div">
<form action=/boaform/formInternetMode method=POST name="Internetmode" target="targetIfr">
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th><% multilang(LANG_OPERATION_MODE); %>:</th>
			<td>
			<% checkWrite("InternetMode"); %>
			</td>
		</tr>
	</table>
</div>	
</td>
      <input type="button" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges()">&nbsp;&nbsp;
	  <input type="hidden" value="/opmode.asp" name="submit-url">
	  <input type="hidden" name="postSecurityFlag" value="">
</td>		 
<br><br>
</form>
</div>

	<br>
	<div id="cntdown_div" align="left" style="display:none;" bgcolor=white;>
	<form name=cntdown_frm><b>
	<input type=text name="cntdwn_text" size=40 style="border:none; height:25px; font-size:20px;" id="cntdwn_text">
	<input type=text name="cntdwn_time" size=5 style="border:none; height:25px; font-size:20px;" id="cntdwn_time">
	</form>
	</div>

	<iframe name="targetIfr" style="display:none"></iframe>
</body>
</html>
