from imagesearchdll import ImageSearchDLL, winactivate, SendKeys, ScriptExitException
from automate_generic import maximize_active_window, close_active_window, start_or_focus_observer, SrchImg, auto_scale_images_for_resolution

ScriptName = "Set Observer alarm settings"
ScriptDescription = "Focus on already started observer. Maximize observer. Open alarms settings window. Select instance. Expand 'Ni Analizer card'. Toggle 'link-up', 'link-down' setting."
    
#put logic in a function so we can call it from external script    
def run_automation():    
    # in case resolution is different, but DPI is the same, there is a chance images can be recycled
    auto_scale_images_for_resolution(1920, 1080, 100, 1.0)
    
    # switch focused application to observer
    start_or_focus_observer()

    #click the alarms icon    
    SrchImg("img/alarms/btn_alarms_0096_0057_0036_0057.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)

    # select the "instance 1" checkbox
    res = SrchImg("img/alarms/chk_inst1_0798_0399_0072_0016.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=5)
    # checkbox can be selected making it a different color, we still want to click it
    if res.found_it == False:
        res = SrchImg("img/alarms/chk_inst1_v2_0797_0398_0074_0018.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=5)

    #click the alarm settings button
    SrchImg("img/alarms/btn_alarm_settings_0757_0640_0177_0020.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)

    # open the "Ni analyzer card .."
    SrchImg("img/alarms/exp_alarm_settings_0678_0491_0128_0011.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=0, click_image=True, click_offset_x=3)

    # click link-down
    SrchImg("img/alarms/chk_link_down_0707_0521_0070_0014.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=6)

    # click link-up
    SrchImg("img/alarms/chk_link_up_0707_0536_0062_0014.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=6)

    # confirm our automation was a success
    SrchImg("img/alarms/confirm1_0701_0519_0083_0033.bmp", wait_appear_timeout=1, accepted_diff_pct=1)

    # save settings and close popup
    SrchImg("img/alarms/btn_ok_probe_alarm_settings_1053_0816_0080_0026.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)

    # save settings and close popup
    SrchImg("img/alarms/btn_ok_alarm_settings_0928_0677_0080_0028.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)
    
    # tell script manager all was fine
    return True

if __name__ == "__main__":
    run_automation()
    