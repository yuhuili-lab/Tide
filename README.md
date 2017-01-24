# Tide (Alpha)
Tide is a m3u8 video downloader written in C++. It first reads the url of m3u8 file supplied by user, then downloads all video segments into a single file for local playback using libcurl.

## Usage
```
./tide url [output_name]
```

## Build
Tide has been tested on macOS.
```
g++ --std=c++14 tide.cpp -o tide -lcurl
```

## Limitations
Since Tide is currently only at Alpha stage, there are a few known limitations and restrictions:
- The m3u8 file supplied through command line needs to be one that contains m3u8 paths for different resolutions. The script will automatically fetch the highest quality determined by the average bandwidth specification. An example would be.
```
#EXTM3U
#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=178271,BANDWIDTH=2785909,CODECS="mp4a.40.5,avc1.4d401f",RESOLUTION=640x360,FRAME-RATE=25.000
360p/play.m3u8
#EXT-X-STREAM-INF:AVERAGE-BANDWIDTH=289071,BANDWIDTH=4869450,CODECS="mp4a.40.5,avc1.640029",RESOLUTION=1280x720,FRAME-RATE=25.000
720p/play.m3u8
```
- All urls in m3u8 files must be relative.
