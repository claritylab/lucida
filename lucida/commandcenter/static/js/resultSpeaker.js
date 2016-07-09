function textToVoice(speakable) {
	var u = new SpeechSynthesisUtterance();
	u.text = speakable;
	u.lang = 'en-US';
	u.rate = 1.0;
	speechSynthesis.speak(u);
}
