package io.antmedia.webrtc.receiver;

import static org.bytedeco.javacpp.avutil.AV_PIX_FMT_YUV420P;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ShortBuffer;

import org.json.simple.JSONObject;
import org.webrtc.AudioSink;
import org.webrtc.AudioTrack;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.PeerConnection;
import org.webrtc.VideoRenderer;
import org.webrtc.VideoRenderer.Callbacks;
import org.webrtc.VideoRenderer.I420Frame;
import org.webrtc.VideoTrack;
import org.webrtc.SessionDescription.Type;

import io.antmedia.webrtc.ConnectionContext;

public class ReceiverConnectionContext extends ConnectionContext {

	FFmpegFrameRecorder recorder;
	protected long startTime;
	
	
	private class VideoRendererCallback implements Callbacks {

		private int frameCount;

		@Override
		public void renderFrame(I420Frame frame) {
			if (startTime == 0) {
				startTime = System.currentTimeMillis();
			}

			frameCount++;

			long pts = (System.currentTimeMillis() - startTime) * 1000;

			Frame frameCV = new Frame(frame.width, frame.height, Frame.DEPTH_UBYTE, 2);

			((ByteBuffer)(frameCV.image[0].position(0))).put(frame.yuvPlanes[0]);
			((ByteBuffer)(frameCV.image[0])).put(frame.yuvPlanes[1]);
			((ByteBuffer)(frameCV.image[0])).put(frame.yuvPlanes[2]);

			try {
				//boolean result = recorder.recordImage(frame.width, frame.height, 2, Frame.DEPTH_UBYTE, frame.yuvStrides[0], AV_PIX_FMT_YUV420P, frame.yuvPlanes);
				//recorder.record(frameCV);
				recorder.recordImage(frameCV.imageWidth, frameCV.imageHeight, frameCV.imageDepth,
						frameCV.imageChannels, frameCV.imageStride, AV_PIX_FMT_YUV420P, pts, frameCV.image);

			} catch (FrameRecorder.Exception e) {
				e.printStackTrace();
			}
			VideoRenderer.renderFrameDone(frame);
		}
	}
	
	private class AudioSinkCallback extends AudioSink {
		
		private int audioFrameCount = 0;

		@Override
		public void onData(byte[] audio_data, int bits_per_sample, int sample_rate, int number_of_channels,
				int number_of_frames) {
			ByteBuffer tempAudioBuffer = ByteBuffer.wrap(audio_data);

			if (startTime == 0) {
				startTime = System.currentTimeMillis();
			}

			audioFrameCount++;

			long timeDiff = (System.currentTimeMillis() - startTime) * 1000;

			if (bits_per_sample == 16) 
			{
				short[] data = new short[number_of_frames * number_of_channels];
				tempAudioBuffer.order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(data, 0, data.length);

				ShortBuffer audioBuffer = ShortBuffer.wrap(data);
				try {
					boolean result = recorder.recordSamples(sample_rate, number_of_channels, timeDiff, audioBuffer);
					if (!result) {
						System.out.println("could not audio sample");
					}
				} catch (FrameRecorder.Exception e) {
					e.printStackTrace();
				}

			}
		}
		
	}

	public ReceiverConnectionContext(FFmpegFrameRecorder recorder) {
		this.recorder = recorder;

		setSdpMediaConstraints(new MediaConstraints());
		getSdpMediaConstraints().mandatory.add(
				new MediaConstraints.KeyValuePair("OfferToReceiveAudio", "true"));
		getSdpMediaConstraints().mandatory.add(
				new MediaConstraints.KeyValuePair("OfferToReceiveVideo", "true"));
	}

	@Override
	public void start() {

	}

	@Override
	public void stop() {
		try {
			if (peerConnection != null) {
				this.peerConnection.close();
				recorder.stop();
				this.peerConnection.dispose();
				this.peerConnectionFactory.dispose();
				peerConnection = null;
			}
		} catch (FrameRecorder.Exception e) {
			e.printStackTrace();
		}
		
		
		JSONObject jsonObject = new JSONObject();
		jsonObject.put("command", "notification");
		jsonObject.put("definition", "publish_finished");

		try {
			wsConnection.send(jsonObject.toJSONString());
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}
	}



	@Override
	public void onAddStream(MediaStream stream) {
		log.warn("onAddStream");

		if (stream.audioTracks != null) {
			AudioTrack audioTrack = stream.audioTracks.getFirst();
			if (audioTrack != null) {
				audioTrack.addSink(new AudioSinkCallback());
			}
		}

		if (stream.videoTracks != null) {

			VideoTrack videoTrack = stream.videoTracks.getFirst();
			if (videoTrack != null) {
				videoTrack.addRenderer(new VideoRenderer(new VideoRendererCallback()));
			}
		}
		
		JSONObject jsonObject = new JSONObject();
		jsonObject.put("command", "notification");
		jsonObject.put("definition", "publish_started");

		try {
			wsConnection.send(jsonObject.toJSONString());
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}
		
		
	}
	
	

	@Override
	public void onSetSuccess() {
		peerConnection.createAnswer(this, getSdpMediaConstraints());
	}

}
