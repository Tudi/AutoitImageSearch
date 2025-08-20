# Name : Set Observer alarm settings
# Description : Focus on already started observer. Maximize observer. Open alarms settings window. Select instance. Expand "Ni Analizer card". Toggle "link-up", "link-down" setting.

from imagesearchdll import ImageSearchDLL, winactivate, SendKeys
import sys
import time

#generic inits
img_dll = ImageSearchDLL()
key_sender = SendKeys()

# helper function. Depending on you are running a pytest or a demo, change this
def SearchImageInRegion(*args, optional_img=False, **kwargs):
    # Forward all arguments directly to the underlying function
    res = img_dll.SearchImageInRegion(*args, **kwargs)
    # this is for me, debugging what is happening
    if res.found_it == False and optional_img == False:
        print(f"Failed to find {res.img_name} {res.hash_diff_pct}% {res.raw}")
        sys.exit(0)
    print(f"Image search result pct match : {res.hash_diff_pct}% at [{res.x},{res.y}] for {res.img_name} raw {res.raw}")
    return res

def maximize_active_window():
    # make sure it's maximized
    time.sleep(0.5) # wait for window to render
    key_sender.send_keys_combination('alt', 'space')
    time.sleep(0.1)  # Small delay for menu to appear
    key_sender.send_key('x')  # Press 'x' for maximize
    time.sleep(0.5) # wait for window to render

# switch focused application to observer
if winactivate("Observer.exe", "C:\Program Files\Observer\Observer.exe", wait_time_startup = 15) == False:
    print("Failed to activate Observer window")
    sys.exit(0)
    
maximize_active_window()

#click the alarms icon    
SearchImageInRegion("img/alarms/btn_alarms_0096_0057_0036_0057.bmp", wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True)

# select the "instance 1" checkbox
res = SearchImageInRegion("img/alarms/chk_inst1_0798_0399_0072_0016.bmp", optional_img = True, wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True, click_offset_x=5)
# checkbox can be selected making it a different color, we still want to click it
if res.found_it == False:
    res = SearchImageInRegion("img/alarms/chk_inst1_v2_0797_0398_0074_0018.bmp", optional_img = True, wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True, click_offset_x=5)

#click the alarm settings button
SearchImageInRegion("img/alarms/btn_alarm_settings_0757_0640_0177_0020.bmp", wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True)

# open the "Ni analyzer card .."
SearchImageInRegion("img/alarms/exp_alarm_settings_0678_0491_0128_0011.bmp", optional_img = True, wait_appear_timeout=1, accepted_similarity_pct=0, click_image=True, click_offset_x=3)

# click link-down
SearchImageInRegion("img/alarms/chk_link_down_0707_0521_0070_0014.bmp", optional_img = True, wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True, click_offset_x=6)

# click link-up
SearchImageInRegion("img/alarms/chk_link_up_0707_0536_0062_0014.bmp", optional_img = True, wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True, click_offset_x=6)

# save settings and close popup
SearchImageInRegion("img/alarms/btn_ok_probe_alarm_settings_1053_0816_0080_0026.bmp", wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True)

# save settings and close popup
SearchImageInRegion("img/alarms/btn_ok_alarm_settings_0928_0677_0080_0028.bmp", wait_appear_timeout=1, accepted_similarity_pct=1, click_image=True)
