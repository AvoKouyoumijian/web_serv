document.getElementById("button").addEventListener("click", () => {
  document.getElementById("button").innerHTML = "hello";
});

var images = [
  "images/home_brazil.jpeg",
  "images/home_sky.jpg",
  "images/home_spain.jpeg",
  "images/home_everest.jpeg",
  "images/home_paris.jpeg",
  "images/home_itely.jpeg",
];
var x = 0;

function displayNextImage() {
  x = (x + 1) % images.length;
  document.getElementById("img").src = images[x];
}

function startTimer() {
  setInterval(displayNextImage, 3000);
}
