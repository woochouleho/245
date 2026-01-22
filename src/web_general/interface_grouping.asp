<%SendWebHeadStr(); %>
<title>Interface Grouping <% multilang(LANG_CONFIGURATION); %></title>

<script>
<% checkWrite("interface_grouping_tabel"); %>

function viewGroup()
{
	with (document.forms[0])
	{
		var i = 0, j = 0, find = 0;
		var group_key = group_select.value;

		for (i = (group_member.options.length - 1); i >= 0; i--)
		{
			group_member.options.remove(i);
		}

		for (i = (group_available.options.length - 1); i >= 0; i--)
		{
			group_available.options.remove(i);
		}

		for (i = 0; i < InterfaceGroupingKey.length; i++)
		{
			if (InterfaceGroupingKey[i] == group_key)
			{
				group_enable.checked = (InterfaceGroupingEnable[i] == "1") ? true:false;
				group_name.value = InterfaceGroupingName[i];
				if (i == 0)
				{
					apply_div.style.display = "none";
				}
				else
				{
					apply_div.style.display = "";
				}

				for (j = 0; j < InterfaceList_Domain.length; j++)
				{
					if (InterfaceGroupingKey[i] == InterfaceList_Group[j])
					{
						group_member.options.add(new Option(InterfaceList_Name[j], InterfaceList_Name[j]));
					}
				}
				find = 1;
				break;
			}
		}

		if (find == 0)
		{
			group_enable.checked = true;
			group_name.value = "";
			apply_div.style.display = "";
		}

		for (j = 0; j < InterfaceList_Domain.length; j++)
		{
			if (InterfaceList_Group[j] == 0)
			{
				group_available.options.add(new Option(InterfaceList_Name[j], InterfaceList_Name[j]));
			}
		}
	}
	return true;
}

function btnRemove()
{
	with (document.forms[0])
	{
		var i = 0;
		for (i = 0; i < group_member.options.length; i++)
		{
			if (group_member.options[i].selected == true)
			{
				group_available.options.add(new Option(group_member.options[i].text, group_member.options[i].value));
			}
		}

		for (i = (group_member.options.length - 1); i >= 0; i--)
		{
			if (group_member.options[i].selected == true)
			{
				group_member.options.remove(i);
			}
		}
	}
	return true;
}

function btnAdd()
{
	with (document.forms[0])
	{
		var i = 0;
		for (i = 0; i < group_available.options.length; i++)
		{
			if (group_available.options[i].selected == true)
			{
				group_member.options.add(new Option(group_available.options[i].text, group_available.options[i].value));
			}
		}

		for (i = (group_available.options.length - 1); i >= 0; i--)
		{
			if (group_available.options[i].selected == true)
			{
				group_available.options.remove(i);
			}
		}
	}
	return true;
}

function build_group_list()
{
	with (document.forms[0])
	{
		var i = 0, j = 0, member_info = "";

		group_member_list.value = "";
		for (i = 0; i < group_member.options.length; i++)
		{
			for (j = 0; j < InterfaceList_Domain.length; j++)
			{
				if (group_member.options[i].value == InterfaceList_Name[j])
				{
					member_info = InterfaceList_Domain[j] + "|" + InterfaceList_ID[j] + "|" + InterfaceList_Name[j] + "|" + InterfaceList_Group[j];
					break;
				}
			}

			if (group_member_list.value != "") group_member_list.value += ",";
			group_member_list.value += member_info;
		}

		group_available_list.value = "";
		for (i = 0; i < group_available.options.length; i++)
		{
			for (j = 0; j < InterfaceList_Domain.length; j++)
			{
				if (group_available.options[i].value == InterfaceList_Name[j])
				{
					member_info = InterfaceList_Domain[j] + "|" + InterfaceList_ID[j] + "|" + InterfaceList_Name[j] + "|" + InterfaceList_Group[j];
					break;
				}
			}

			if (group_available_list.value != "") group_available_list.value += ",";
			group_available_list.value += member_info;
		}
	}
	return true;
}

function delClick(group_key)
{
	with (document.forms[0])
	{
		var i = 0;

		if (!confirm('<% multilang(LANG_CONFIRM_DELETE_ONE_ENTRY); %>'))
		{
			return false;
		}
		else
		{
			group_action.value = "0";
			group_number.value = group_key;

			group_select.value = group_key;
			viewGroup();

			for (i = 0; i < group_member.options.length; i++)
			{
				group_available.options.add(new Option(group_member.options[i].text, group_member.options[i].value));
			}

			for (i = (group_member.options.length - 1); i >= 0; i--)
			{
				group_member.options.remove(i);
			}

			build_group_list();
			postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
			submit();
			return true;
		}
	}
	return true;
}

function saveClick()
{
	with (document.forms[0])
	{
		group_action.value = "1";
		group_number.value = group_select.value;

		if (group_enable.checked)
		{
			group_enable.disabled = true;
			group_enable_value.value = "1";
		}
		else
		{
			group_enable.disabled = true; 
			group_enable_value.value = "0";
		}

		build_group_list();
	}

	postTableEncrypt(document.forms[0].postSecurityFlag, document.forms[0]);
	return true;
}

function formLoad()
{
	viewGroup();
	return true;
}
</script>
</head>

<body onLoad="formLoad();">
	<div class="intro_main ">
		<p class="intro_title">Interface Grouping <% multilang(LANG_CONFIGURATION); %></p>
	</div>

	<form action="/boaform/formInterfaceGrouping" method="POST" name="InterfaceGroupingform">
		<div class="data_common data_common_notitle">
			<table>
				<tr>
					<th width="30%"><% multilang(LANG_SELECT); %>:</th>
					<td width="70%">
						<select name="group_select" id="group_select" onChange="viewGroup();">
							<script language="javascript">
							var i = 0;
							for (i = 1; i < InterfaceGroupingKey.length; i++)
							{
								document.write("<option value=\"" + InterfaceGroupingKey[i] + "\">" + InterfaceGroupingName[i] + "</option>");
							}
							document.write("<option value=\"-1\">New Group</option>");
							</script>
						</select>
					</td>
				</tr>
				<tr>
					<th width="30%"><% multilang(LANG_ENABLE); %>:</th>
					<td width="70%"><input type="checkbox" name="group_enable" id="group_enable" value="1"></td>
				</tr>
				<tr>
					<th width="30%"><% multilang(LANG_NAME); %>:</th>
					<td width="70%"><input type="text" name="group_name" id="group_name" size="35" maxlength="64"></td>
				</tr>
			</table>
			<table>
				<tr align="center">
					<td><b><% multilang(LANG_GROUPED_INTERFACES); %></b></td>
					<td></td>
					<td><b><% multilang(LANG_AVAILABLE_INTERFACES); %></b></td>
				</tr>
				<tr align="center">
					<td>
						<select multiple name="group_member" id="group_member" size="8" style="width: 120px; height: 150px"></select>
					</td>
					<td>
						<input type="button" name="rmbtn" value="->" onClick="btnRemove();" style="width: 30px; height: 30px">
						<br><br>
						<input type="button" name="adbtn" value="<-" onClick="btnAdd();" style="width: 30px; height: 30px">
					</td>
					<td>
						<select multiple name="group_available" id="group_available" size="8" style="width: 120px; height: 150px"></select>
					</td>
				</tr>
			</table>
		</div>

		<div id="apply_div" class="btn_ctl">
			<input class="link_bg" type="submit" value="<% multilang(LANG_APPLY_CHANGES); %>"  onClick="return saveClick()">
			<input type="hidden" name="group_action" value="1">
			<input type="hidden" name="group_number" value="0">
			<input type="hidden" name="group_enable_value" value="0">
			<input type="hidden" name="group_member_list" value="">
			<input type="hidden" name="group_available_list" value="">
			<input type="hidden" value="/interface_grouping.asp" name="submit-url">
			<input type="hidden" name="postSecurityFlag" value="">
		</div>

		<div class="column">
			<div class="column_title">
				<div class="column_title_left"></div>
				<p>Interface Grouping Table</p>
				<div class="column_title_right"></div>
			</div>
			<div class="data_common data_vertical">
				<table>
				<tr>
					<th align=center width="25%"><% multilang(LANG_NAME); %></th>
					<th align=center width="15%"><% multilang(LANG_STATUS); %></th>
					<th align=center width="50%"><% multilang(LANG_INTERFACES); %></th>
					<th align=center width="10%"><% multilang(LANG_ACTION); %></th>
				</tr>
				<script language="javascript">
				var i = 0, j = 0, member_list = "";
				for (i = 0; i < InterfaceGroupingKey.length; i++)
				{
					document.write("<tr>");
					document.write("<td>" + InterfaceGroupingName[i] + "</td>");
					document.write("<td>" + ((InterfaceGroupingEnable[i] == "0") ? "<% multilang(LANG_DISABLE); %>":"<% multilang(LANG_ENABLE); %>") + "</td>");

					member_list = "";
					for (j = 0; j < InterfaceList_Domain.length; j++)
					{
						if (InterfaceGroupingKey[i] == InterfaceList_Group[j])
						{
							if (member_list != "") member_list += ",";
							member_list += InterfaceList_Name[j];
						}
					}
					document.write("<td>" + member_list + "</td>");

					if (i > 0)
						document.write("<td><image onClick=\"delClick(" + InterfaceGroupingKey[i] + ")\" title=\"Delete\" src=\"graphics/del.gif\"></td>");
					else
						document.write("<td></td>");
					document.write("</tr>");
				}
				</script>
				</table>
			</div>
		</div>
	</form>
</body>
</html>
