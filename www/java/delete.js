function deleteFile() {
	const filename = document.getElementById("filename").value.trim();

	if (!filename) {
		alert("Veuillez entrer un nom de fichier.");
		return;
	}

	fetch("/delete", {
		method: "DELETE",
		headers: {
			"Content-Type": "application/json"
		},
		body: JSON.stringify({ filename: filename })
	})
	.then(response => response.text())
	.then(data => {
		alert(data);
	})
	.catch(error => {
		alert("Erreur : " + error);
	});
}
