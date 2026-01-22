<% SendWebHeadStr();%>
<title><% multilang(LANG_WAN_PORT_SET); %></title>

<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT>

var wan_port_set=<% checkWrite("wan_port_set"); %>
function LoadSetting()
{
	
	if(wan_port_set)
	{
		document.wan_port_set.wan_port_sync[0].checked=true;
		document.wan_port_set.wan_port_sync[1].checked=false;
	}
	else
	{
		document.wan_port_set.wan_port_sync[0].checked=false;
		document.wan_port_set.wan_port_sync[1].checked=true;
	}
}
function wan_port_set_func(value)
{
	if(value ==0) //disable sync config
	{
		document.wan_port_set.wan_port_sync[0].checked=false;
	}
	else //enable sync config
	{
		document.wan_port_set.wan_port_sync[1].checked=false;
	}

}

function saveChanges(obj)
{	
	obj.isclick = 1;
	postTableEncrypt(document.wan_port_set.postSecurityFlag, document.wan_port_set);
	return true;
}
</SCRIPT>
</head>


<BODY onLoad="LoadSetting();">
	<div class="intro_main ">
		<p class="intro_title"><% multilang(LANG_WAN_PORT_SET); %></p>
		<p class="intro_content"><% multilang(LANG_PAGE_DESC_CONFIGURE_INTERNET_WAN_PORT_SETTING); %></p>
	</div>
<div id="wan_port_set_1">
<form action=/boaform/formWanPortSet method=POST name="wan_port_set" >
<div class="data_common data_common_notitle">
	<table>
		<tr>
			<th><% multilang(LANG_WAN_PORT_SET); %>:</th>
			<td>
			<% checkWrite("SelectionWanPort"); %>
			</td>
		</tr>
	</table>
</div>	
</td>
      <input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>"  name="save" onClick="return saveChanges(this)">&nbsp;&nbsp;
	  <input type="hidden" value="/wan_port_set.asp" name="submit-url">
	  <input type="hidden" name="postSecurityFlag" value="">
</td>		 

</form>
</div>
	

</body>

</html>


