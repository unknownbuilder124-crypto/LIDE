/* ==========================================================================
   Search & Navigation Logic
   ========================================================================== */

// Determines if input is a URL or a Google Search
function handleSearch() {
    const query = document.getElementById('search').value.trim();
    if (!query) return;
    
    // Simple check: if it contains a dot and no spaces, treat as URL
    if (query.includes('.') && !query.includes(' ')) {
        // Force HTTPS if protocol is missing
        if (!query.startsWith('http://') && !query.startsWith('https://')) {
            window.location.href = 'https://' + query;
        } else {
            window.location.href = query;
        }
    } else {
        // Otherwise, perform a Google Search
        window.location.href = 'https://www.google.com/search?q=' + encodeURIComponent(query);
    }
}

// Listen for Enter key in the search box
document.getElementById('search').addEventListener('keypress', function(e) {
    if (e.key === 'Enter') handleSearch();
});

// Auto-focus the search box when the page loads
window.addEventListener('load', function() {
    document.getElementById('search').focus();
});

/* ==========================================================================
   Dropdown Menu Logic
   ========================================================================== */

// Toggles the visibility of the three-dot menu options
function toggleDropdown(event) {
    event.preventDefault(); // Prevents the link from navigating
    event.stopPropagation(); // Prevents the click from bubbling up
    
    // Close any other open dropdowns first
    document.querySelectorAll('.dropdown.active').forEach(dropdown => {
        if (dropdown !== event.currentTarget.parentElement) {
            dropdown.classList.remove('active');
        }
    });

    // Toggle the clicked one
    const dropdown = event.currentTarget.parentElement;
    dropdown.classList.toggle('active');
}

// Close dropdowns if the user clicks anywhere else on the page
window.onclick = function(event) {
    if (!event.target.closest('.dropdown')) {
        document.querySelectorAll('.dropdown.active').forEach(dropdown => {
            dropdown.classList.remove('active');
        });
    }
}

/* ==========================================================================
   Tab Management Logic (Add & Edit)
   ========================================================================== */

// Helper function to extract domain for the Favicon API
function getDomain(url) {
    try {
        return new URL(url).hostname;
    } catch (e) {
        return ''; 
    }
}

// Handles editing an existing tab's name and URL
function editTab(event) {
    event.preventDefault();
    event.stopPropagation();

    // Find the specific tab elements
    const tabElement = event.target.closest('.nav-item');
    const textElement = tabElement.querySelector('.tab-text');
    const iconElement = tabElement.querySelector('img.icon');
    
    const currentName = textElement.textContent;
    const currentUrl = tabElement.getAttribute('href');

    // Prompt user for new info
    const newName = prompt("Edit tab name:", currentName);
    if (newName === null || newName.trim() === "") return;

    let newUrl = prompt("Edit website URL:", currentUrl);
    if (newUrl === null || newUrl.trim() === "") return;

    // Ensure valid URL protocol
    if (!newUrl.startsWith('http://') && !newUrl.startsWith('https://')) {
        newUrl = 'https://' + newUrl;
    }

    // Update the DOM elements
    textElement.textContent = newName.trim();
    tabElement.setAttribute('href', newUrl.trim());
    
    // Update the Favicon using Google's API
    const domain = getDomain(newUrl);
    if (iconElement && domain) {
        iconElement.src = `https://www.google.com/s2/favicons?domain=${domain}&sz=32`;
    }

    // Close the dropdown menu
    document.querySelectorAll('.dropdown.active').forEach(d => d.classList.remove('active'));
}

// Creates a new tab and appends it to the sidebar
function addNewTab() {
    const tabName = prompt("Enter tab name (e.g., Wikipedia):");
    if (!tabName) return; 

    let tabUrl = prompt("Enter website URL (e.g., wikipedia.org):");
    if (!tabUrl) return;

    // Ensure valid URL protocol
    if (!tabUrl.startsWith('http://') && !tabUrl.startsWith('https://')) {
        tabUrl = 'https://' + tabUrl;
    }

    const domain = getDomain(tabUrl);
    const container = document.getElementById('today-tabs-container');
    
    // Generate the HTML for the new tab, including the Favicon API call
    const newTabHTML = `
        <a href="${tabUrl}" target="_blank" class="nav-item">
            <img src="https://www.google.com/s2/favicons?domain=${domain}&sz=32" class="icon" alt="icon">
            <span class="tab-text">${tabName}</span>
            <div class="dropdown">
                <button class="tab-options-btn" onclick="toggleDropdown(event)">
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="1"></circle><circle cx="12" cy="5" r="1"></circle><circle cx="12" cy="19" r="1"></circle></svg>
                </button>
                <div class="dropdown-content">
                    <div class="dropdown-item" onclick="event.preventDefault(); alert('Pinned to top!')">Pin Tab</div>
                    <div class="dropdown-item" onclick="event.preventDefault(); alert('Added to Bookmarks!')">Add to Bookmarks</div>
                    <div class="dropdown-item" onclick="editTab(event)">Edit Tab</div>
                    <div class="dropdown-item" onclick="event.preventDefault(); this.closest('.nav-item').remove()">Close Tab</div>
                </div>
            </div>
        </a>
    `;
    
    // Inject the new tab into the sidebar
    container.insertAdjacentHTML('beforeend', newTabHTML);
}

/* ==========================================================================
   Customization & LocalStorage Logic (Background & Avatar)
   ========================================================================== */

const bgUpload = document.getElementById('bg-upload');
const avatarUpload = document.getElementById('avatar-upload');
const btnSettings = document.getElementById('btn-settings');
const profilePic = document.getElementById('profile-pic');

// Applies the background image and triggers the CSS 'has-bg' class for Dark Mode
function applyBackground(imgUrl) {
    document.body.style.backgroundImage = `url(${imgUrl})`;
    document.body.classList.add('has-bg');
}

// Trigger hidden file inputs when clicking the UI buttons
btnSettings.addEventListener('click', () => bgUpload.click());
profilePic.addEventListener('click', () => avatarUpload.click());

// Handle Background Image Upload
bgUpload.addEventListener('change', function() {
    const file = this.files[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = function(e) {
            const imgUrl = e.target.result;
            applyBackground(imgUrl);
            localStorage.setItem('voidfox-bg', imgUrl); // Save to browser storage
        }
        reader.readAsDataURL(file);
    }
});

// Handle Profile Avatar Upload
avatarUpload.addEventListener('change', function() {
    const file = this.files[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = function(e) {
            const imgUrl = e.target.result;
            profilePic.style.backgroundImage = `url(${imgUrl})`;
            localStorage.setItem('voidfox-avatar', imgUrl); // Save to browser storage
        }
        reader.readAsDataURL(file);
    }
});

// Load saved settings when the page is refreshed or reopened
window.addEventListener('DOMContentLoaded', () => {
    const savedBg = localStorage.getItem('voidfox-bg');
    if (savedBg) applyBackground(savedBg);

    const savedAvatar = localStorage.getItem('voidfox-avatar');
    if (savedAvatar) profilePic.style.backgroundImage = `url(${savedAvatar})`;
});