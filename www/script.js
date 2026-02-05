document.getElementById('btn').addEventListener('click', () => {
    const msg = document.getElementById('msg');
    const now = new Date();
    msg.textContent = "Button clicked at: " + now.toLocaleTimeString();
});

