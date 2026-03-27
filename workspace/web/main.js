import addressData from "./data/address.json" with { type: "json" };

const sortedAddress = Object.keys(addressData).toSorted();
const apiEndpoint = "/api/config";

const level1Select = document.getElementById("level1");
const level2Select = document.getElementById("level2");
const level3Select = document.getElementById("level3");
const configForm = document.getElementById("configForm");
const wifiIdInput = document.getElementById("wifiId");
const wifiPasswordInput = document.getElementById("wifiPassword");

const address = {
    level1: null,
    level2: null,
    level3: null,
}

const wifi = {
    id: null,
    password: null,
}

function populateLevel1() {
    const documentFragment = document.createDocumentFragment();
    sortedAddress.forEach(option => {
        const opt = document.createElement("option");
        opt.value = option;
        opt.textContent = option;
        documentFragment.appendChild(opt);
    });
    level1Select.appendChild(documentFragment);
}

function populateLevel2(level1) {
    const level2Data = Object.keys(addressData[level1]).toSorted();
    level2Select.innerHTML = '<option value="">시/군/구</option>';
    const documentFragment = document.createDocumentFragment();
    level2Data.forEach(option => {
        const opt = document.createElement("option");
        opt.value = option;
        opt.textContent = option;
        documentFragment.appendChild(opt);
    });
    level2Select.appendChild(documentFragment);
    level2Select.disabled = false;
     level3Select.innerHTML = '<option value="">동/읍/면</option>';
     level3Select.disabled = true;
}

function populateLevel3(level1, level2) {
    const level3Data = addressData[level1][level2].toSorted();
    level3Select.innerHTML = '<option value="">동/읍/면</option>';
    const documentFragment = document.createDocumentFragment();
    level3Data.forEach(option => {
        const opt = document.createElement("option");
        opt.value = option;
        opt.textContent = option;
        documentFragment.appendChild(opt);
    });
    level3Select.appendChild(documentFragment);
    level3Select.disabled = false;
}

level1Select.addEventListener("change", (event) => {
    address.level1 = event.target.value;
    if (address.level1) {
        populateLevel2(address.level1);
    } else {
        level2Select.innerHTML = '<option value="">시/군/구</option>';
        level3Select.innerHTML = '<option value="">동/읍/면</option>';
    }
});

level2Select.addEventListener("change", (event) => {
    address.level2 = event.target.value;
    if (address.level2) {
        populateLevel3(address.level1, address.level2);
    } else {
        level3Select.innerHTML = '<option value="">동/읍/면</option>';
    }
});

level3Select.addEventListener("change", (event) => {
    address.level3 = event.target.value;
});

wifiIdInput.addEventListener("input", (event) => {
    wifi.id = event.target.value;
});

wifiPasswordInput.addEventListener("input", (event) => {
    wifi.password = event.target.value;
});

configForm.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (wifi.id && wifi.password && address.level1 && address.level2 && address.level3) {
        const isConfirmed = confirm(`제출하시겠습니까?
- 공유기 ID: ${wifi.id}, 
- 공유기 Password: ${wifi.password},
- 주소: ${address.level1} ${address.level2} ${address.level3}
        `);

        if(isConfirmed) {
            const response = await fetch(apiEndpoint, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    wifi,
                    address
                })
            });
            if (response.ok) {
                alert("설정이 성공적으로 완료되었습니다.");
            } else {
                alert("설정이 실패했습니다.");
            }

        }
    }
});


document.addEventListener("DOMContentLoaded", () => {
    populateLevel1();
});