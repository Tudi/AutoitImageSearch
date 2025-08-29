from imagesearchdll import ImageSearchDLL, winactivate, SendKeys, ScriptExitException
from automate_generic import maximize_active_window, close_active_window, start_or_focus_observer, SrchImg, key_sender, auto_scale_images_for_resolution

#generic inits
ScriptName = "Set Observer certificate analisys settings"
ScriptDescription = "Focus on already started observer. Maximize observer. Open network trending. open settings."
    
#put logic in a function so we can call it from external script    
def run_automation():
    # in case resolution is different, but DPI is the same, there is a chance images can be recycled
    auto_scale_images_for_resolution(1920, 1080, 100, 1.0)
    
    # switch focused application to observer
    start_or_focus_observer();
    
    #start Network Trending
    res = SrchImg("img/certif_set/btn_ntw_tr_0237_0051_0062_0074.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)
    # clicking the button opened a sub button. In case the image was not found, it's because NT is already opened
    if res.found_it == True:
        res = SrchImg("img/certif_set/btn_ntw_tr_0238_0127_0164_0018.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)
        #wait for NT to start up. Alost instant ?
        time.sleep(0.5)
        
    # search the newly appeared network trending tab and make sure it is the one selected
    res = SrchImg("img/certif_set/tab_nt_tr_inactive_0427_0146_0100_0019.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, extend_search_left=2000, extend_search_right=2000)

    # click settings
    res = SrchImg("img/certif_set/btn_nt_settings_0421_0165_0068_0018.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)

    #set checkbox 1
    res = SrchImg("img/certif_set/chk_certif_0667_0413_0108_0015.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=7)
    #set checkbox 2
    res = SrchImg("img/certif_set/chk_collect_ip_0686_0430_0135_0018.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=7)
    #set checkbox 3
    res = SrchImg("img/certif_set/chk_ja3_0686_0453_0153_0017.bmp", optional_img = True, wait_appear_timeout=1, accepted_diff_pct=1, click_image=True, click_offset_x=7)

    # confirm our automation was a success
    SrchImg("img/certif_set/confirm1_0663_0414_0179_0055.bmp", wait_appear_timeout=1, accepted_diff_pct=1)

    #save the setting changes
    res = SrchImg("img/certif_set/btn_ok_1164_0846_0080_0024.bmp", wait_appear_timeout=1, accepted_diff_pct=1, click_image=True)

    #close observer
    close_active_window()
    
    # tell script manager all was fine
    return True

if __name__ == "__main__":
    run_automation()
