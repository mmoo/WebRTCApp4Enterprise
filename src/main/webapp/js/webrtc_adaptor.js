

/**
 * 
 * @returns
 */


function WebRTCAdaptor(initialValues) 
{
	var thiz = this;
	thiz.peerconnection_config = null;
	thiz.sdp_constraints = null;
	thiz.remotePeerConnection = null;
	thiz.webSocketAdaptor = null;
	thiz.streamName = null;
	thiz.videoTrackSender = null;
	thiz.audioTrackSender = null;

	thiz.isPlayMode = false;
	thiz.debug = false;

	for(var key in initialValues) {
		if(initialValues.hasOwnProperty(key)) {
			this[key] = initialValues[key];
		}
	}

	thiz.localVideo = document.getElementById(thiz.localVideoId);
	thiz.remoteVideo = document.getElementById(thiz.remoteVideoId);

	if (!("WebSocket" in window)) {
		console.log("WebSocket not supported.");
		callbackError("WebSocketNotSupported");
		return;
	}

	if (!this.isPlayMode)  // if it is in play mode, do not get user media
	{
		if (typeof thiz.mediaConstraints.video != "undefined" && thiz.mediaConstraints.video != "false") 
		{
			//var media_video_constraint = { video: thiz.mediaConstraints.video };
			navigator.mediaDevices.getUserMedia(thiz.mediaConstraints)
			.then(function(stream){

				//this trick, getting audio and video separately, make us add or remove tracks on the fly
				var audioTrack = stream.getAudioTracks();
				if (audioTrack.length > 0) {
					stream.removeTrack(audioTrack[0]);
				}

				//now get only audio to add this stream
				if (typeof thiz.mediaConstraints.audio != "undefined" && thiz.mediaConstraints.audio != "false") {
					var media_audio_constraint = { audio: thiz.mediaConstraints.audio};
					navigator.mediaDevices.getUserMedia(media_audio_constraint)
					.then(function(audioStream) {
						stream.addTrack(audioStream.getAudioTracks()[0]);
						thiz.gotStream(stream);
					})
					.catch(function(error) {
						thiz.callbackError(error.name);		
					});
				}
			})
			.catch(function(error) {
				thiz.callbackError(error.name);		
			});
		}
		else {
			var media_audio_constraint = { video: thiz.mediaConstraints.audio };
			navigator.mediaDevices.getUserMedia(media_audio_constraint)
			.then(thiz.gotStream)
			.catch(function(error) {
				thiz.callbackError(error.name);		
			});
		}
	}
	else {
		if (thiz.webSocketAdaptor == null || thiz.webSocketAdaptor.isConnected() == false) {
			thiz.webSocketAdaptor = new WebSocketAdaptor();
		}
	}


	this.publish = function (streamName) {
		thiz.streamName = streamName;

		var jsCmd;

		jsCmd = {
				command : "publish",
				streamName : streamName,
		};


		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}

	this.play = function (streamName) {
		thiz.streamName = streamName;
		var jsCmd =
		{
				command : "play",
				streamName : thiz.streamName,
		}

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}

	this.stop = function(streamName) {
		thiz.closePeerConnection();

		var jsCmd = {
				command : "stop",
		};

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));


	}

	this.join = function(streamName) {
		thiz.streamName = streamName;

		var jsCmd = {
				command : "join",
				streamName : streamName,
		};


		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}

	this.leave = function () {

		var jsCmd;
		{

			jsCmd = {
					command : "leave",
					streamName: thiz.streamName,
			};
		}

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));

		thiz.closePeerConnection();
	}

	this.gotStream = function (stream) {

		thiz.localStream = stream;
		thiz.localVideo.srcObject = stream;
		if (thiz.webSocketAdaptor == null || thiz.webSocketAdaptor.isConnected() == false) {
			thiz.webSocketAdaptor = new WebSocketAdaptor();
		}

	};

	this.onTrack = function(event) {
		console.log("onTrack");
		thiz.remoteVideo.srcObject = event.streams[0];
	}

	this.iceCandidateReceived = function(event) {
		if (event.candidate) {

			jsCmd = {
					command : "takeCandidate",
					streamName : thiz.streamName,
					label : event.candidate.sdpMLineIndex,
					id : event.candidate.sdpMid,
					candidate : event.candidate.candidate
			};
			
			if (thiz.debug) {
				console.log("sending ice candiate: " + JSON.stringify(event.candidate));
			}

			thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
		}
	}


	this.initPeerConnection = function() {
		if (thiz.remotePeerConnection == null) {
			thiz.remotePeerConnection = new RTCPeerConnection(thiz.peerconnection_config);
			if (!thiz.isPlayMode) {
				thiz.remotePeerConnection.addStream(thiz.localStream);
			}
			thiz.remotePeerConnection.onicecandidate = thiz.iceCandidateReceived;
			thiz.remotePeerConnection.ontrack = thiz.onTrack;
		}
	}

	this.closePeerConnection = function() {
		if (thiz.remotePeerConnection != null
				&& thiz.remotePeerConnection.signalingState != "closed") {
			thiz.remotePeerConnection.close();
			thiz.remotePeerConnection = null;
		}
	}

	this.signallingState = function() {
		if (thiz.remotePeerConnection != null) {
			return thiz.remotePeerConnection.signalingState;
		}
		return null;
	}

	this.iceConnectionState = function() {
		if (thiz.remotePeerConnection != null) {
			return thiz.remotePeerConnection.iceConnectionState;
		}
		return null;
	}

	this.gotDescription = function(configuration) {
		thiz.remotePeerConnection.setLocalDescription(configuration);

		jsCmd = {
				command : "takeConfiguration",
				streamName : thiz.streamName,
				type : configuration.type,
				sdp : configuration.sdp

		};
		
		if (thiz.debug) {
			console.log("local sdp: " );
			console.log(configuration);
		}

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}


	this.turnOffLocalCamera = function() {
		if (thiz.remotePeerConnection != null) {
			var senders = thiz.remotePeerConnection.getSenders();
			var videoTrackSender = null;


			for (index in senders) {
				if (senders[index].track.kind == "video") {
					videoTrackSender = senders[index];
					break;
				}
			}
			if (videoTrackSender != null) {
				thiz.remotePeerConnection.removeTrack(videoTrackSender);
				thiz.localStream.removeTrack(videoTrackSender.track);
				thiz.localVideo.srcObject = thiz.localStream;

				thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
				.then(thiz.gotDescription)
				.catch(function () {
					console.log("create offer error");
				});
			}


		}
		else {
			this.callbackError("NoActiveConnection");
		}
	}

	this.turnOnLocalCamera = function() {
		if (thiz.remotePeerConnection != null) {
			var senders = thiz.remotePeerConnection.getSenders();
			var videoTrackSender = null;


			for (index in senders) {
				if (senders[index].track.kind == "video") {
					videoTrackSender = senders[index];
					break;
				}
			}
			if (videoTrackSender == null) {
				var value = true;
				if (typeof thiz.mediaConstraints.video != "undefined" && thiz.mediaConstraints.video != "false")
				{
					value = thiz.mediaConstraints.video;
				}
				navigator.mediaDevices.getUserMedia({video:value})
				.then(function(stream) {
					thiz.localStream.addTrack(stream.getVideoTracks()[0]);
					thiz.localVideo.srcObject = thiz.localStream;
					thiz.remotePeerConnection.addTrack(thiz.localStream.getVideoTracks()[0], thiz.localStream);

					thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
					.then(thiz.gotDescription)
					.catch(function () {
						console.log("create offer error");
					});
				})
				.catch(function(error) {
					thiz.callbackError(error.name);		
				});
			}
			else {
				this.callbackError("VideoAlreadyActive");
			}


		}
		else {
			this.callbackError("NoActiveConnection");
		}
	}

	this.muteLocalMic = function() {
		if (thiz.remotePeerConnection != null) {
			var senders = thiz.remotePeerConnection.getSenders();
			var audioTrackSender = null;

			for (index in senders) {
				if (senders[index].track.kind == "audio") {
					audioTrackSender = senders[index];
					break;
				}
			}
			if (audioTrackSender != null) {
				thiz.remotePeerConnection.removeTrack(audioTrackSender);
				thiz.localStream.removeTrack(audioTrackSender.track);
				thiz.localVideo.srcObject = thiz.localStream;

				thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
				.then(thiz.gotDescription)
				.catch(function () {
					console.log("create offer error");
				});
			}
			else {
				this.callbackError("AudioAlreadyNotActive");
			}

		}
		else {
			this.callbackError("NoActiveConnection");
		}
	}

	/**
	 * if there is audio it calls callbackError with "AudioAlreadyActive" parameter
	 */
	this.unmuteLocalMic = function() {
		if (thiz.remotePeerConnection != null) {
			var senders = thiz.remotePeerConnection.getSenders();
			var audioTrackSender = null;

			for (index in senders) {
				if (senders[index].track.kind == "audio") {
					audioTrackSender = senders[index];
					break;
				}
			}
			if (audioTrackSender == null) {

				navigator.mediaDevices.getUserMedia({audio:thiz.mediaConstraints.audio})
				.then(function(stream) {
					thiz.localStream.addTrack(stream.getAudioTracks()[0]);
					thiz.localVideo.srcObject = thiz.localStream;
					thiz.remotePeerConnection.addTrack(thiz.localStream.getAudioTracks()[0], thiz.localStream);

					thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
					.then(thiz.gotDescription)
					.catch(function () {
						console.log("create offer error");
					});
				})
				.catch(function(error) {
					thiz.callbackError(error.name);		
				});

			}
			else {
				this.callbackError("AudioAlreadyActive");
			}

		}
		else {
			this.callbackError("NoActiveConnection");
		}
	}

	//this.WebSocketAdaptor = function() {
	function WebSocketAdaptor() {
		var wsConn = new WebSocket(thiz.websocket_url);

		var connected = false;

		wsConn.onopen = function() {
			if (thiz.debug) {
				console.log("websocket connected");
			}
			
			connected = true;
			thiz.callback("initialized");
		}

		this.send = function(text) {
            
			if (wsConn.readyState == 0 || wsConn.readyState == 2 || wsConn.readyState == 3) {
				thiz.callbackError("WebSocketNotConnected");
				return;
			}
			wsConn.send(text);
		}

		this.isConnected = function() {
			return connected;
		}

		wsConn.onmessage = function(event) {
			obj = JSON.parse(event.data);

			if (obj.command == "start") {
				thiz.initPeerConnection();

				if (thiz.debug) {
					console.log("received start command");
				}
				

				thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
				.then(thiz.gotDescription)
				.catch(function () {
					if (thiz.debug) {
						console.log("create offer error");
					}
				});
			}
			else if (obj.command == "takeCandidate") {

				thiz.initPeerConnection();
				var candidate = new RTCIceCandidate({
					sdpMLineIndex : obj.label,
					candidate : obj.candidate
				});
				thiz.remotePeerConnection.addIceCandidate(candidate);
				if (thiz.debug) {
					console.log("received ice candidate: ");
					console.log(candidate);
				}

			} else if (obj.command == "takeConfiguration") {

				thiz.initPeerConnection();
				thiz.remotePeerConnection
				.setRemoteDescription(new RTCSessionDescription({
					sdp : obj.sdp,
					type : obj.type
				}));
				if (thiz.debug) {
					console.log("received remote description type:" );
					console.log(obj);
				}
				
				if (obj.type == "offer") {
					thiz.remotePeerConnection.createAnswer(thiz.sdp_constraints)
					.then(thiz.gotDescription)
					.catch(function() {
						console.log("create answer error");
					});
				}
			}
			else if (obj.command == "stop") {
				thiz.closePeerConnection();
			}
			else if (obj.command == "error") {
				thiz.callbackError(obj.definition);
			}
			else if (obj.command == "notification") {
				thiz.callback(obj.definition);
			}

		}

		wsConn.onerror = function(error) {
			console.log(" error occured: " + JSON.stringify(error));
			thiz.callbackError(error)
		}

		wsConn.onclose = function(event) {
			connected = false;

			console.log("connection closed.");
			thiz.callback("closed", event);
		}
	};
}

