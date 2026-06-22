class CopyrightText extends HTMLElement {
    connectedCallback() {
        const name = this.getAttribute('name');
        const year = new Date().getFullYear();
        this.innerHTML = `<p>${name} &copy;${year}</p>`;
    }
}
customElements.define('copyright-text', CopyrightText);