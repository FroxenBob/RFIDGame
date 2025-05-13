async function updateTable() {
  try {
    const res = await fetch("/data");
    const data = await res.json();
    const tbody = document.getElementById("table-body");
    tbody.innerHTML = "";

    data.forEach((row, index) => {
      const tr = document.createElement("tr");
      tr.innerHTML = `
        <td>${index + 1}</td>
        <td>${row.uid}</td>
        <td>${row.time}</td>
        <td>${row.source}</td>
      `;
      tbody.appendChild(tr);
    });
  } catch (err) {
    console.error("⚠ Failed to update table:", err);
  }
}

updateTable();                   // initial call
setInterval(updateTable, 1000);  // every 1 sec