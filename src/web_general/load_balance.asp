<% SendWebHeadStr();%>
<title>RIP <% multilang(LANG_CONFIGURATION); %></title>

<SCRIPT>
function show_binding_parameter()
{
	var binding_type = get_by_id("binding_type");

	if(binding_type.value ==1){
		//binding by mac
		get_by_id("show_mac").style.display = "";
		get_by_id("show_start_ip").style.display = "none";
		get_by_id("show_end_ip").style.display = "none";
	}
	else
	{
		//binding by ip
		get_by_id("show_start_ip").style.display = "";
		get_by_id("show_end_ip").style.display = "";
		get_by_id("show_mac").style.display = "none";
	}
}

function addClick(obj)
{
	var total_lan_value = document.staticLoadBalance.total_lan_num.value;
	var check_box_index;
	var tmp_value;
	var tmp_checked;

	if(document.staticLoadBalance.balance_enable.value==0)
	{
		alert("static load balance is disabled!");
		return false;
	}

	for(var i = 0; i < total_lan_value; i++)
	{
		check_box_index = "set" + i;
		tmp_checked = document.getElementById(check_box_index).checked;
		if(tmp_checked)
			document.getElementById(check_box_index).value = 1;
		else
			document.getElementById(check_box_index).value = 0;

	}
	obj.isclick = 1;
	postTableEncrypt(document.staticLoadBalance.postSecurityFlag, document.staticLoadBalance);
	
	return true;
}

function addDynClick(obj)
{
	
	obj.isclick = 1;
	postTableEncrypt(document.DynmaicBalance.postSecurityFlag, document.DynmaicBalance);
	
	return true;
}

function deleteClick(obj)
{
	if ( !confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.staticLoadBalanceDel.postSecurityFlag, document.staticLoadBalanceDel);
		return true;
	}
}

function on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.staticLoadBalance.postSecurityFlag, document.staticLoadBalance);
	return true;
}

function total_on_submit(obj)
{
	obj.isclick = 1;
	postTableEncrypt(document.DynmaicBalance.postSecurityFlag, document.DynmaicBalance);
	return true;
}


function deleteAllClick(obj)
{
	if ( !confirm('Do you really want to delete the all entries?') ) {
		return false;
	}
	else{
		obj.isclick = 1;
		postTableEncrypt(document.staticLoadBalanceDel.postSecurityFlag, document.staticLoadBalanceDel);
		return true;
	}
}

function typeChange()
{
	with(document.DynmaicBalance)
	{
		if(dynamic_balance_type.value == 1)
		{
			connect_mode.style.display = "none";
			bandwidth_mode.style.display = "block";
		}
		else{
			connect_mode.style.display = "block";
			bandwidth_mode.style.display = "none";
		}
	}		
}

function get_ct_selected_value()
{
	var ct_id;
	var ct_selected_value;
	var num = <%checkWrite("wan_dev_num");%>;

	for(var i =0; i<num; i++)
	{
		ct_id = "ct_name_"+i;
		ct_selected_value = "ct_selected_value_"+i;
		document.getElementById(ct_id).selectedIndex = document.getElementById(ct_selected_value).value;
	}
}

function get_balance_type()
{
	var ct_id;
	var ct_selected_value;
	var bd_id;
	var bd_selected_value;
	var num = <%checkWrite("wan_dev_num");%>;

	for(var i =0; i<num; i++)
	{
		ct_id = "ct_type_"+i;
		ct_selected_value = "ct_type_value_"+i;
		document.getElementById(ct_id).selectedIndex = document.getElementById(ct_selected_value).value;

		bd_id = "bd_type_"+i;
		bd_selected_value = "bd_type_value_"+i;
		document.getElementById(bd_id).selectedIndex = document.getElementById(bd_selected_value).value;
	}
}

function Status_Selection()
{
	if (document.staticLoadBalance.daemon[0].checked) {
		document.staticLoadBalance.ext_if.disabled = true;

		if(typeof(document.staticLoadBalance.tr_064_sw) != "undefined")
		{
			document.staticLoadBalance.tr_064_sw[0].disabled = true;
			document.staticLoadBalance.tr_064_sw[1].disabled = true;
		}
	}
	else {
		document.staticLoadBalance.ext_if.disabled = false;

		if(typeof(document.staticLoadBalance.tr_064_sw) != "undefined")
		{
			document.staticLoadBalance.tr_064_sw[0].disabled = false;
			document.staticLoadBalance.tr_064_sw[1].disabled = false;
		}
	}
}
function auto_balance_selection()
{
	var num = <%checkWrite("wan_dev_num");%>;
	var dyn_ct_type;
	var dyn_ct_select_wan;
	var dyn_bd_type;
	var dyn_bd_select_wan;

	if (document.DynmaicBalance.dynamic_balance_enable[0].checked) {
		document.DynmaicBalance.dynamic_balance_type.disabled = true;
		for(var i =0; i<num; i++)
		{
			dyn_ct_type = "ct_type_"+i;
			dyn_ct_select_wan = "ct_name_"+i;
			dyn_bd_type = "bd_type_"+i;
			dyn_bd_select_wan = "bd_name_"+i;
			document.getElementById(dyn_ct_type).disabled = true;
			document.getElementById(dyn_ct_select_wan).disabled = true;
			document.getElementById(dyn_bd_type).disabled = true;
			document.getElementById(dyn_bd_select_wan).disabled = true;
		}		
	}
	else{
		document.DynmaicBalance.dynamic_balance_type.disabled = false;
		for(var i =0; i<num; i++)
		{
			dyn_ct_type = "ct_type_"+i;
			dyn_ct_select_wan = "ct_name_"+i;
			dyn_bd_type = "bd_type_"+i;
			dyn_bd_select_wan = "bd_name_"+i;
			document.getElementById(dyn_ct_type).disabled = false;
			document.getElementById(dyn_ct_select_wan).disabled = false;
			document.getElementById(dyn_bd_type).disabled = false;
			document.getElementById(dyn_bd_select_wan).disabled = false;
		}
	}
}

function manual_balance_selection()
{
	var num = <%checkWrite("lan_info_num");%>;
	var manual_selected_name;
	var manual_wan_itf;

	if (document.staticLoadBalance.balance_enable[0].checked) {
		for(var i =0; i<num; i++)
		{
			manual_selected_name = "set"+i;
			document.getElementById(manual_selected_name).disabled = true;
		}
	}
	else{
		for(var i =0; i<num; i++)
		{
			manual_selected_name = "set"+i;
			document.getElementById(manual_selected_name).disabled = false;
		}
	}
}
</SCRIPT>
</head>

<body>
<div class="intro_main ">
	<p class="intro_title"><% multilang(LANG_LOAD_BALANCE); %></p>
	<p class="intro_content"><% multilang(LANG_ENABLE_STATIC_LOAD_BALANCE_YOU_CAN_BING_WAN_BY_SRC_IP_OR_SRC_MAC); %></p>
</div>

<form action=/boaform/formDynmaicBalance method=POST name="DynmaicBalance">
&nbsp;
<div class="data_common data_common_notitle">
<div class="column_title">
			<div class="column_title_left"></div>
			<p>Global Settings</p>
			<div class="column_title_right"></div>
</div>
		<table>
			<tr>
			<th width="20%"><% multilang(LANG_TOTAL_LOAD_BALANCE); %></th>
			<td width="55%">
				<input type="radio" value="0" name="total_balance_enable" <% checkWrite("total_balance0"); %>><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="total_balance_enable" <% checkWrite("total_balance1"); %>><% multilang(LANG_ENABLE); %>
			</td>
			<td>
				<input class="inner_btn" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply_total" onClick="return total_on_submit(this)">&nbsp;&nbsp;
			</td>
			</tr>
		</table>
</div>
&nbsp;&nbsp;
<div class="data_common data_common_notitle">
	<table>
		<div class="column_title">
			<div class="column_title_left"></div>
			<p>Auto Load Balance</p>
			<div class="column_title_right"></div>
		</div>
		<tr>
			<th width="20%"><% multilang(LANG_DYNAMIC_LOAD_BALANCE); %></th>
			<td width="80%">
				<input type="radio" value="0" name="dynamic_balance_enable" <% checkWrite("dyn_balance0"); %> onClick="auto_balance_selection()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="dynamic_balance_enable" <% checkWrite("dyn_balance1"); %> onClick="auto_balance_selection()"><% multilang(LANG_ENABLE); %>
			</td>
		</tr>
		<tr>
			<th width="20%">Policy:</th>
			<td width="80%">
				<select size="1" style="width:170px" name="dynamic_balance_type" <%checkWrite("dyn_balance_enable"); %> onChange="typeChange()">
				<option value="1"><% multilang(LANG_BROADBAND_LOAD_BALANCE); %></option>
				<!--<option value="2"><% multilang(LANG_CONNECTION_LOAD_BALANCE); %></option>-->
				</select>
			</td>
		</tr>
	</table>
	<div id='connect_mode' style="display:none;">
	<table>
 		<tr> 
			<th width="20%"><% multilang(LANG_WAN_INTERFACE); %></th>
			<th width="40%"><% multilang(LANG_DYN_LOAD_BALANCE_TYPE); %></th>
			<th width="15%"><% multilang(LANG_WAN_PORT); %></th>
			<th width="25%"><% multilang(LANG_PROPORTION_OF_TRAFFIC); %></th>
		</tr>
			<% rtk_set_connect_load_balance(); %>
	</table>
	</div>
	<div id='bandwidth_mode' style="display:block;">
	<table>
 		<tr> 
			<th width="20%"><% multilang(LANG_WAN_INTERFACE); %></th>
			<th width="40%"><% multilang(LANG_DYN_LOAD_BALANCE_TYPE); %></th>
			<th width="15%"><% multilang(LANG_WAN_PORT); %></th>
			<th width="25%"><% multilang(LANG_PROPORTION_OF_TRAFFIC); %></th>
		</tr>
			<% rtk_set_bandwidth_load_balance(); %>
	</table>
	</div>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="addDynBalance" onClick="return addDynClick(this)">
	<input type="hidden" value="/load_balance.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
	<script>
		get_ct_selected_value();
		get_balance_type();
		document.DynmaicBalance.dynamic_balance_type.value = <% checkWrite("dyn_balance_type"); %>;
		typeChange();
	</script>
</form>

<form action=/boaform/formStaticBalance method=POST name="staticLoadBalance">
&nbsp;&nbsp;&nbsp;
<div class="data_common data_common_notitle">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p>Manual Load Balance</p>
		<div class="column_title_right"></div>
	</div>
	<table>
		<tr>
			<th width="20%"><% multilang(LANG_STATIC_LOAD_BALANCE); %>:</th>
			<td width="55%">
				<input type="radio" value="0" name="balance_enable" <% checkWrite("sta_balance0"); %> onClick="manual_balance_selection()"><% multilang(LANG_DISABLE); %>&nbsp;&nbsp;
				<input type="radio" value="1" name="balance_enable" <% checkWrite("sta_balance1"); %> onClick="manual_balance_selection()"><% multilang(LANG_ENABLE); %>
			</td>
			<td>
				<input class="inner_btn" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="apply" onClick="return on_submit(this)">&nbsp;&nbsp;
			</td>
		</tr>
	</table>
	<table>
 		<tr> 
 			<th width="20%"><% multilang(LANG_ENABLE_STATIC_LOAD_BALANCE); %></th>
			<th width="20%"><% multilang(LANG_LOAD_BALANCE_DEV_NAME); %></th>
			<th width="20%"><% multilang(LANG_MAC_ADDRESS); %></th>
			<th width="15%"><% multilang(LANG_LAN_PORT); %></th>
			<th width="25%"><% multilang(LANG_WAN_INTERFACE); %></th>
		</tr>
			<% rtk_set_lan_sta_load_balance(); %>
	</table>
</div>
<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_ADD); %>" name="addStaBalance" onClick="return addClick(this)">
	<input type="hidden" value="/load_balance.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>
</form>

<form action=/boaform/formStaticBalance method=POST name="staticLoadBalanceDel">
&nbsp;&nbsp;&nbsp;
<div class="data_common data_common_notitle">
	<div class="column_title">
		<div class="column_title_left"></div>
		<p><% multilang(LANG_STATIC_LOAD_BALANCE_TABLE); %></p>
		<div class="column_title_right"></div>
	</div>

	<table>
		<% sta_balance_list(); %>
	</table>

</div>

<div class="btn_ctl">
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_SELECTED); %>" name="deleteSelStaBalance" onClick="return deleteClick(this)">&nbsp;&nbsp;
	<input class="link_bg" type="submit" value="<% multilang(LANG_DELETE_ALL); %>" name="deleteAllStaBalance" onClick="return deleteAllClick(this)">&nbsp;&nbsp;&nbsp;
	<input type="hidden" value="/load_balance.asp" name="submit-url">
	<input type="hidden" name="postSecurityFlag" value="">
</div>

</form>
<br><br>
</body>

</html>


