<% SendWebHeadStr();%>
<title><% multilang(LANG_LAN_INTERFACE_SETTINGS); %></title>
<SCRIPT>
function resetClick()
{
	document.tcpip.reset;
}

function saveChanges()
{
	var lpm1 = 0;
	var lpm2 = 0;
	var RTKIPv6Enable=<% checkWrite("rtk_ipv6_enable"); %>;

	if (!checkHostIP(document.tcpip.ip, 1))
		return false;
	if (!checkNetmask(document.tcpip.mask, 1))
		return false;

	// Magician 2013/08/23: LAN port mask.
	with (document.forms[0])
	{
		if(typeof chk_port_mask1 != 'undefined' && chk_port_mask1 != null){
			for (var i = 0; i < chk_port_mask1.length; i++) {
				if (chk_port_mask1[i].checked == true)
					lpm1 |= (0x1 << i);
			}
			lan_port_mask1.value = lpm1;
		}
		if(typeof chk_port_mask2 != 'undefined' && chk_port_mask2 != null){
			for (var i = 0; i < chk_port_mask2.length; i++) {
				if (chk_port_mask2[i].checked == true)
					lpm2 |= (0x1 << i);
			}
			lan_port_mask2.value = lpm2;
		}
	    if(RTKIPv6Enable == "yes")
	    {
		    if (ipv6_addr.value =="" || ipv6_addr.value =="::") {
			    alert("<% multilang(LANG_LAN_IPV6_ADDRESS_CANNOT_BE_EMPTY_FORMAT_IS_IPV6_ADDRESS_FOR_EXAMPLE_3FFE_501_FFFF_100_1); %>");
			    ipv6_addr.focus();
	    		return false;
		    } else {
		    	if ( validateKeyV6IP(ipv6_addr.value) == 0) {
		    		alert("<% multilang(LANG_INVALID_LAN_IPV6_IP); %>");
		    		ipv6_addr.focus();
		    		return false;
		    	}
		    }

            if ( ipv6landnsmode.value == 2 ){  //static
		    	if(Ipv6Dns1.value == "" && Ipv6Dns2.value == "" )  //Both DNS setting is NULL
		    	{
		    		Ipv6Dns1.focus();
		    		alert("IPv6 DNS Address\"" + Ipv6Dns1.value + "\"is Invalid! Please Re-type.");
		    		return false;
		    	}
		    	else if (Ipv6Dns1.value != "" || Ipv6Dns2.value != ""){
		    		if(Ipv6Dns1.value != "" ){
		    			if (! isUnicastIpv6Address( Ipv6Dns1.value) ){
		    					alert("Primary IPv6 DNS Address\"" + Ipv6Dns1.value + "\"is Invalid! Please Re-type.");
		    					Ipv6Dns1.focus();
		    					return false;
		    			}
		    		}
		    		if(Ipv6Dns2.value != "" ){
		    			if (! isUnicastIpv6Address( Ipv6Dns2.value) ){
		    					alert("Backup IPv6 DNS Address\"" + Ipv6Dns1.value + "\"is Invalid! Please Re-type.");
		    					Ipv6Dns2.focus();
		    					return false;
		    			}
		    		}
		    	}
		    }
            if ( ipv6lanprefixmode.value == 1 ){
		        if(lanIpv6prefix.value == "" )
		        {
		    	    lanIpv6prefix.focus();
		    	    alert("IPv6 Address\"" + lanIpv6prefix.value + "\"is Invalid Prefix address! Please Re-type. For example 3ffe:501:ffff:100::/64");
		    	    return false;
		        }
		        else if ( validateKeyV6Prefix(lanIpv6prefix.value) == 0) { //check if is valid ipv6 address
		    	    alert("Invalid IPv6 Prefix Length!");
		    	    lanIpv6prefix.focus();
		    	    return false;
		        }
	        }
		}  
		save.isclick = 1;
	}
	// End Magician

	<% checkIP2(); %>
	
	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);	
	return true;
}

function disableRadioGroup (radioArrOrButton)
{
  if (radioArrOrButton.type && radioArrOrButton.type == "radio") {
 	var radioButton = radioArrOrButton;
 	var radioArray = radioButton.form[radioButton.name];
  }
  else
 	var radioArray = radioArrOrButton;
 	radioArray.disabled = true;
 	for (var b = 0; b < radioArray.length; b++) {
 	if (radioArray[b].checked) {
 		radioArray.checkedElement = radioArray[b];
 		break;
	}
  }
  for (var b = 0; b < radioArray.length; b++) {
 	radioArray[b].disabled = true;
 	radioArray[b].checkedElement = radioArray.checkedElement;
  }
}

function updateState()
{
  if (document.tcpip.wlanDisabled.value == "ON") {

    disableRadioGroup(document.tcpip.BlockEth2Wir);

  }
}

function dnsModeChange()
{
	with (document.forms[0])
    {
    	var dns_mode =ipv6landnsmode.value;
    	
    	v6dns_WANConnection.style.display = 'none';	
     	v6dns_Staic1.style.display = 'none';
     	v6dns_Staic2.style.display = 'none';
    	switch(dns_mode){
    		case '0': //HGWProxy
    				break;
    		case '1': //WANConnection
    				v6dns_WANConnection.style.display = '';
    				break;
    		case '2': //Static
        			v6dns_Staic1.style.display = '';
        			v6dns_Staic2.style.display = '';
    				if(Ipv6Dns1.value == "::") //clear the value
    					Ipv6Dns1.value ="";
    				if(Ipv6Dns2.value == "::")
    					Ipv6Dns2.value ="";
    
    				break;		
    	}
    }
}

function prefixModeChange()
{
	with (document.forms[0])
    {
    	var prefix_mode = ipv6lanprefixmode.value;
    	
    	v6delegated_WANConnection.style.display = 'none';
    	staticipv6prefix.style.display = 'none';
    
    	switch(prefix_mode){
    		case '0': //WANDelegated
    				v6delegated_WANConnection.style.display = '';
    				break;
    		case '1': //Static
    				staticipv6prefix.style.display = '';
    				break;
    	}
    }
}
<% DHCPClientScript(); %>
<% TcpipLanOninit(); %>
<% lanScript(); %>
</SCRIPT>
</head>

<BODY onLoad="on_init();">
	<div class="intro_main ">
		<p class="intro_title"><% multilang(LANG_LAN_INTERFACE_SETTINGS); %></p>
		<p class="intro_content"><% multilang(LANG_PAGE_DESC_CONFIG_DEVICE_LAN_INTERFACE); %></p>
	</div>

	<form action=/boaform/formTcpipLanSetup method=POST name="tcpip">
		<div class="data_common data_common_notitle">
			<table>
				<input type=hidden name="wlanDisabled" value=<% wlanStatus(); %>>
				<tr>
				  <th width="30%"><% multilang(LANG_INTERFACE); %><% multilang(LANG_NAME); %>:</th>
				  <td width="70%">br0</td>
				</tr>
				<tr>
				  <th width="30%"><% multilang(LANG_IP_ADDRESS); %>:</th>
				  <td width="70%"><input type="text" name="ip" id="ip" size="15" maxlength="15" value=<% getInfo("lan-ip"); %>></td>
				</tr>
				<tr>
				  <th width="30%"><% multilang(LANG_SUBNET_MASK); %>:</th>
				  <td width="70%"><input type="text" name="mask" id="mask" size="15" maxlength="15" value="<% getInfo("lan-subnet"); %>"></td>
				</tr>

				<!-- Hide VID setting temporarily -->
				<tr style="display:none">
					<th width="30%"><% multilang(LANG_VLAN_ID); %>:</th>
					<td width="70%"><input type="text" name="lan_vlan_id1" size="15" maxlength="15" value="<% getInfo("lan_vlan_id1"); %>"></td>
				</tr>
				<tbody id='ipv6_settings_div' style="display:none">
                    <tr>
                      <th width="30%"><% multilang(LANG_IPV6_ADDRESS); %>:&nbsp;&nbsp;</th>
                      <td width="70%"><input type="text" name="ipv6_addr" size="30" maxlength="60" value=<% checkWrite("lanipv6addr"); %>></td>
                    </tr>
                    <tr>
                      <th width="30%"><% multilang(LANG_IPV6_DNS_MODE); %>:&nbsp;&nbsp;</th>
                      <td width="70%"><select name="ipv6landnsmode" onChange="dnsModeChange()">
                     <option value="0">HGWProxy</option>
                   <option value="1">WANConnection</option>
                     <option value="2">Static</option>
                    </select>
                      </td>
                    </tr>
                    <tr id='v6dns_WANConnection' style="display:none">
                        <th width="30%"><% multilang(LANG_WAN_INTERFACE); %>:&nbsp;&nbsp;</th>
                        <td width="70%"><select name="if_dns_wan" > <% if_wan_list("rtv6"); %> </select></td>
                    </tr>
                 	<tr id='v6dns_Staic1' style="display:none">
                  		<th width="30%"><% multilang(LANG_IPV6); %> <% multilang(LANG_DNS1); %>:&nbsp;&nbsp;</th>
                  		<td width="70%"><input type="text" name="Ipv6Dns1" size="30" maxlength="60" value=<% getInfo("wan-dnsv61");%>></td>
                 	</tr>
                 	<tr id='v6dns_Staic2' style="display:none">
                        <th width="30%"><% multilang(LANG_IPV6); %> <% multilang(LANG_DNS2); %>:&nbsp;&nbsp;</th>
                  		<td width="70%"><input type="text" name="Ipv6Dns2" size="30" maxlength="60" value=<% getInfo("wan-dnsv62");%>></td>
                 	</tr>
                    <tr>
                     	<th width="30%"><% multilang(LANG_PREFIX_MODE); %>:&nbsp;&nbsp;</th>
                     	<td width="70%">
						<select name="ipv6lanprefixmode" onChange="prefixModeChange()">
                         	<option value="0">WANDelegated </option>
                       		<option value="1">Static</option>
                      	</select>
                        </td>
                    </tr>
                 	<tr id='v6delegated_WANConnection' style="display:none">
                        <th width="30%"><% multilang(LANG_WAN_INTERFACE); %>:&nbsp;&nbsp;</th>
                        <td width="70%"><select name="if_delegated_wan" > <% if_wan_list("rtv6"); %> </select></td>
                    </tr>
                    <tr id="staticipv6prefix" style="display:none">
                     	<th><% multilang(LANG_PREFIX); %>:&nbsp;&nbsp;</th>
                     	<td><input type="text" name="lanIpv6prefix" size="30" maxlength="60" value=<% checkWrite("lanipv6prefix"); %>></td>
                    </tr>
               </tbody>
			</table>
		</div>
		<% lan_setting(); %>

		<div class="btn_ctl">
			<input type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>" name="save" onClick="return saveChanges()" class="link_bg">&nbsp;&nbsp;
			<!--input type="reset" value="Undo" name="reset" onClick="resetClick()"-->
			<input type="hidden" value="/tcpiplan.asp" name="submit-url">
			<input type="hidden" name="lan_port_mask1" value=0>
			<input type="hidden" name="lan_port_mask2" value=0>
			<input type="hidden" name="postSecurityFlag" value="">
		</div>
		<script>
		<% initPage("lan"); %>
		updateState();
		var  RTKIPv6Enable=<% checkWrite("rtk_ipv6_enable"); %>;
		if (RTKIPv6Enable == "yes")
    		{
			document.getElementById("ipv6_settings_div").style.display ='';
		}
		</script>
	</div>
	</form>
<br><br>
</body>
</html>
