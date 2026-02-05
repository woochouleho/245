/*******************************************************************************
 *                                                                             *
 * (c) COPYRIGHT 2019 HFR                                                      *
 *                                                                             *
 * ALL RIGHTS RESERVED                                                         *
 *                                                                             *
 * This software is the property of "HFR" and is furnished                     *
 * under license by "HFR". This software may be used only in                   *
 * accordance with the terms of said license. This copyright notice may not    *
 * be removed, modified or obliterated without the prior written permission    *
 * of "HFR".                                                                   *
 *                                                                             *
 * This software may not be copied, transmitted, provided to or otherwise      *
 * made available to any other person, company, corporation or other entity    *
 * except as specified in the terms of said license.                           *
 *                                                                             *
 * No right, title, ownership or other interest in the software is hereby      *
 * granted or transferred.                                                     *
 *                                                                             *
 * The information contained herein is subject to change without notice and    *
 * should not be constructed as a commitment by "HFR".                         *
 *                                                                             *
 ******************************************************************************/

/*****************************************************************************
 * File Name: popup.js
 * Description:
 *
 * Author: taba
 * E-Mail: jmkim@hfrnet.com
 * Version: 1.0
 * Created Date: 20191204
 ****************************************************************************/

;(function($){
	var Popup = function(newconfig){
		var self = this;
		this.config = {
			'type': 'info',
			'title': 'Popup Title',
			'text': 'Popup Message',
			'align': 'center',
			'fontsize' : 14,
			'autohide': false,
			'showtime': 3000,
			'closeCallBack': null,
			'submitvalue': '예',
			'submitCallBack': null,
			'cancelvalue': '아니요'
		}
		
		this.extendconfig = $.extend(this.config, newconfig);
		this.maskDom = '';
		this.basicDom = '<div class="popup_cover"></div>\n'+
								'<div class="popup confirm_popup" style="display: block;">\n' +
								'	<div class="inner">\n'+
								'		<h1>확인</h1>\n' +
								'		<div class="pop_cont">\n'+
								'			<p class="txt_1"></p>\n'+
								'				<button class="popup_close"><img src="img/gnb_close.png" alt="팝업닫기"></button>\n' +
								'		</div>\n'+
								'	</div>\n' +								
								'</div>\n';

		this.renderPopup();

		$('.popup_close').click(function(){
			if(self.extendconfig.closeCallBack != null ){
				self.extendconfig.closeCallBack();
			}
			self.destroy();
		})


		$('.popup_submit').click(function(){
			if(self.extendconfig.submitCallBack != null){
				self.extendconfig.submitCallBack();
			}
			self.destroy();
		})


		$('.popup_cancel').click(function(){
			if(self.extendconfig.closeCallBack != null ){
				self.extendconfig.closeCallBack();
			}
			self.destroy();
		})

	}
	Popup.prototype = {

		renderPopup: function(){
			var self = this;

			var config = this.extendconfig,
					type = config.type,
					title = config.title,
					text = config.text,
					align = config.align,
					fontsize =  config.fontsize,
					autohide = config.autohide,
					showtime = config.showtime,
					submitvalue = config.submitvalue,
					cancelvalue = config.cancelvalue;


			$('body').append(this.maskDom, this.basicDom);
			switch (type) {
				case 'info':
					break;
				case 'confirm':
					var submitDom = '';
						submitDom = '<div class="txt_c">' +
													'<button class="basic-btn01 btn-blue-bg popup_submit">'+ submitvalue +'</button>' +
													'<button class="basic-btn01 btn-gray-bg2 ml_10 popup_cancel">' + cancelvalue + '</button>' +
												'</div>';

					$('.pop_cont').append(submitDom);
					break;
					
				case 'alert' :
					var submitDom = '';
						submitDom = '<div class="txt_c">' +
													'<button class="basic-btn01 btn-blue-bg popup_submit">'+ submitvalue +'</button>' +
												'</div>';

					$('.pop_cont').append(submitDom);
					break;
					
				default:
					break;
			}
				
			var popupDom = $('.inner');
			
			popupDom.find('.txt_1').append(text);
			popupDom.find('.txt_1').css('font-size', fontsize);
			popupDom.find('.txt_1').css('text-align', align);


			if(type === 'info' && autohide === true){
				if(isNaN(showtime)){
					showtime = 3000
				}
				var showTimer = setTimeout(function(){
					self.destroy();
				}, showtime)
			}


			$('.popup_cover').show();
			$('.confirm_popup').show();
		},

		destroy: function(){
			$('.popup_cover').fadeOut(0, function(){
				$(this).remove();
			})
			$('.confirm_popup').fadeOut(0, function(){
				$(this).remove();
			});
		}
	}
	window.Popup = Popup;
})(jQuery);


function webAlert(message) {
	var popup = new Popup({
			'type': 'alert',
			'title': '설정오류',
			'fontsize' : 14,
			'text': message,
			'submitvalue': '확인',
			'autohide': true,
			'cancelbutton': true,
		});
}

function simpleConfirm(message) {
		var result = false;
		var popup = new Popup({
			'type': 'confirm',
			'title': '설정 확인',
			'text': message,
			'cancelbutton': true,
			'closeCallBack': function(){
				result = false;
			},
			'submitCallBack': function(){
				result = true;
			}
		});
		
	return result;
}
