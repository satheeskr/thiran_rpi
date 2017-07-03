from __future__ import print_function
import time
import pychromecast

chromecasts = pychromecast.get_chromecasts()
[cc.device.friendly_name for cc in chromecasts]
['Satty Music']

cast = next(cc for cc in chromecasts if cc.device.friendly_name == "Satty Music")
# Wait for cast device to be ready
cast.wait()
mc = cast.media_controller
mc.block_until_active()
mc.stop()
#mc.disconnect()
