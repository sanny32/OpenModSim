// Screenshot modal functionality
document.addEventListener('DOMContentLoaded', function() {
    const modal = document.getElementById('imageModal');
    const modalImage = document.getElementById('modalImage');
    const prevBtn = document.getElementById('modalPrev');
    const nextBtn = document.getElementById('modalNext');
    const carouselPrevBtn = document.getElementById('screenshotPrev');
    const carouselNextBtn = document.getElementById('screenshotNext');
    const viewport = document.getElementById('screenshotViewport');
    const track = document.getElementById('screenshotTrack');
    const dotsContainer = document.getElementById('screenshotDots');
    const screenshots = Array.from(document.querySelectorAll('.screenshot-item img'));

    let currentIndex = 0;
    let carouselIndex = 0;
    let pointerStartX = 0;
    let pointerDeltaX = 0;
    let isPointerActive = false;

    function getSlidesPerView() {
        if (window.innerWidth <= 768) return 1;
        if (window.innerWidth <= 968) return 2;
        return 3;
    }

    function getMaxCarouselIndex() {
        return Math.max(0, screenshots.length - getSlidesPerView());
    }

    function renderDots() {
        if (!dotsContainer) return;
        const pageCount = getMaxCarouselIndex() + 1;
        dotsContainer.innerHTML = '';

        for (let i = 0; i < pageCount; i += 1) {
            const dot = document.createElement('button');
            dot.type = 'button';
            dot.className = 'screenshot-dot';
            dot.setAttribute('aria-label', `Go to screenshot page ${i + 1}`);
            dot.addEventListener('click', function() {
                setCarouselIndex(i);
            });
            dotsContainer.appendChild(dot);
        }
    }

    function updateCarousel() {
        if (!track || screenshots.length === 0) return;

        const maxIndex = getMaxCarouselIndex();
        carouselIndex = Math.max(0, Math.min(carouselIndex, maxIndex));

        const item = screenshots[carouselIndex]?.closest('.screenshot-item');
        const offset = item ? item.offsetLeft : 0;
        track.style.transform = `translateX(-${offset}px)`;

        if (carouselPrevBtn) carouselPrevBtn.disabled = carouselIndex === 0;
        if (carouselNextBtn) carouselNextBtn.disabled = carouselIndex === maxIndex;

        const dots = dotsContainer ? Array.from(dotsContainer.querySelectorAll('.screenshot-dot')) : [];
        dots.forEach(function(dot, index) {
            dot.classList.toggle('active', index === carouselIndex);
        });
    }

    function setCarouselIndex(index) {
        carouselIndex = index;
        updateCarousel();
    }

    function showModal(index) {
        currentIndex = index;
        modalImage.src = screenshots[currentIndex].src;
        modalImage.alt = screenshots[currentIndex].alt;
        modal.classList.add('active');
        document.body.style.overflow = 'hidden';
        updateNavButtons();
    }

    function updateNavButtons() {
        prevBtn.classList.toggle('hidden', currentIndex === 0);
        nextBtn.classList.toggle('hidden', currentIndex === screenshots.length - 1);
        modalImage.style.cursor = 'pointer';
    }

    // Add click event to all screenshots
    screenshots.forEach((screenshot, index) => {
        screenshot.addEventListener('click', function() {
            if (window.innerWidth <= 768) return;
            showModal(index);
        });
    });

    if (carouselPrevBtn) {
        carouselPrevBtn.addEventListener('click', function() {
            setCarouselIndex(carouselIndex - 1);
        });
    }

    if (carouselNextBtn) {
        carouselNextBtn.addEventListener('click', function() {
            setCarouselIndex(carouselIndex + 1);
        });
    }

    if (viewport) {
        viewport.addEventListener('pointerdown', function(e) {
            isPointerActive = true;
            pointerStartX = e.clientX;
            pointerDeltaX = 0;
        });

        viewport.addEventListener('pointermove', function(e) {
            if (!isPointerActive) return;
            pointerDeltaX = e.clientX - pointerStartX;
        });

        function finishPointerGesture() {
            if (!isPointerActive) return;
            isPointerActive = false;

            if (Math.abs(pointerDeltaX) > 50) {
                if (pointerDeltaX < 0) setCarouselIndex(carouselIndex + 1);
                if (pointerDeltaX > 0) setCarouselIndex(carouselIndex - 1);
            }

            pointerDeltaX = 0;
        }

        viewport.addEventListener('pointerup', finishPointerGesture);
        viewport.addEventListener('pointercancel', finishPointerGesture);
        viewport.addEventListener('pointerleave', finishPointerGesture);
    }

    prevBtn.addEventListener('click', function(e) {
        e.stopPropagation();
        if (currentIndex > 0) showModal(currentIndex - 1);
    });

    nextBtn.addEventListener('click', function(e) {
        e.stopPropagation();
        if (currentIndex < screenshots.length - 1) showModal(currentIndex + 1);
    });

    // Click on image — go to next screenshot, or close on last
    modalImage.addEventListener('click', function(e) {
        e.stopPropagation();
        if (currentIndex < screenshots.length - 1) showModal(currentIndex + 1);
        else closeModal();
    });

    // Close modal when clicking outside the image
    modal.addEventListener('click', function(e) {
        if (e.target === modal) closeModal();
    });

    // Keyboard navigation
    document.addEventListener('keydown', function(e) {
        if (!modal.classList.contains('active')) {
            if (e.key === 'ArrowLeft') setCarouselIndex(carouselIndex - 1);
            if (e.key === 'ArrowRight') setCarouselIndex(carouselIndex + 1);
            return;
        }
        if (e.key === 'Escape') closeModal();
        if (e.key === 'ArrowLeft' && currentIndex > 0) showModal(currentIndex - 1);
        if (e.key === 'ArrowRight' && currentIndex < screenshots.length - 1) showModal(currentIndex + 1);
    });

    function closeModal() {
        modal.classList.remove('active');
        document.body.style.overflow = '';
    }

    renderDots();
    updateCarousel();
    window.addEventListener('resize', function() {
        renderDots();
        updateCarousel();
    });
});
