from imagesearchdll import ImageSearchDLL, winactivate, SendKeys, ScriptExitException
import sys
import time

img_dll = ImageSearchDLL()
key_sender = SendKeys()
log_fnc=print

def set_log_fnc(logger):
    log_fnc=logger
    
def maximize_active_window():
    # make sure it's maximized
    time.sleep(0.5) # wait for window to render
    key_sender.send_keys_combination('alt', 'space')
    time.sleep(0.1)  # Small delay for menu to appear
    key_sender.send_key('x')  # Press 'x' for maximize
    time.sleep(0.5) # wait for window to render
    
    
def close_active_window(press_ok_confirm=True):
    time.sleep(0.5) # wait for window to render
    key_sender.send_keys_combination('alt', 'space')
    time.sleep(0.1)  # Small delay for menu to appear
    key_sender.send_key('C')  # Press 'C' for close
    time.sleep(0.5) # wait for window to render
    if press_ok_confirm:
        key_sender.send_key('enter')
    # do we confirm it is closed ?    
    
def start_or_focus_observer():
    # switch focused application to observer
    if winactivate("Observer.exe", "C:/Program Files/Observer/Observer.exe", wait_time_startup = 4) == False:           
        # wait until observer starts up. At this point the window is not maximized, so we do not know for sure where the probe icon will be
        key_sender.send_key('enter') # development VM has a popup
        SrchImg("img/generic/obs_splash_1178_0637_0039_0052.bmp", wait_appear_timeout=5, accepted_diff_pct=1, search_radius_for_auto_taget=100)
        key_sender.send_key('enter') # development VM has a popup
        SrchImg("img/generic/obs_splash_1178_0637_0039_0052.bmp", wait_dissapear_timeout=15, accepted_diff_pct=1, search_radius_for_auto_taget=100)
        # check again if we can locate / focus on it
        if winactivate("Observer.exe", "C:/Program Files/Observer/Observer.exe", wait_time_startup = 4) == False:
                raise ScriptExitException("Failed to activate Observer window", False)

        
    maximize_active_window()    

# helper function. Depending on you are running a pytest or a demo, change this
def SrchImg(*args, optional_img=False, **kwargs):
    # Forward all arguments directly to the underlying function
    res = img_dll.SearchImageInRegion(*args, **kwargs)
    
    automation_step_passed = True
    # this is for me, debugging what is happening
    if ('wait_dissapear_timeout' in kwargs) and kwargs['wait_dissapear_timeout'] > 0:
        if res.found_it == True:
            msg = f"Img did not dissapear {res.img_name} {res.hash_diff_pct}% diff"
            automation_step_passed = False
    
    elif res.found_it == False and optional_img == False:
        msg = f"Failed to find {res.img_name} {res.hash_diff_pct}% diff"
        automation_step_passed = False
        
    if automation_step_passed == False:
        log_fnc(msg)
        # fail this script since we failed to find the image we were looking for
        raise ScriptExitException(msg, False)
    log_fnc(f"ImgSearch:{res.hash_diff_pct}% diff at [{res.x},{res.y}] for {res.img_name}")
    return res    
    
# wrapper over dll interface 
def auto_scale_images_for_resolution(width,height,zoom,scale):
    img_dll.auto_scale_images_for_resolution(width,height,zoom,scale)