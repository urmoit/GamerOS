// Advanced animations and interactions for GamerOS website

document.addEventListener('DOMContentLoaded', function() {
    // Initialize all animations
    initAnimations();
    initScrollEffects();
    initHoverEffects();
    initLoadingEffects();
});

function initAnimations() {
    // Smooth scrolling for navigation links
    const navLinks = document.querySelectorAll('nav ul li a');
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            const href = this.getAttribute('href');
            if (href.startsWith('#')) {
                e.preventDefault();
                const target = document.querySelector(href);
                if (target) {
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            }
        });
    });

    // Advanced Intersection Observer for scroll animations
    const observerOptions = {
        threshold: 0.1,
        rootMargin: '0px 0px -100px 0px'
    };

    const observer = new IntersectionObserver((entries) => {
        entries.forEach((entry, index) => {
            if (entry.isIntersecting) {
                setTimeout(() => {
                    entry.target.classList.add('animate-in');
                }, index * 150); // Stagger animations
            }
        });
    }, observerOptions);

    // Observe elements for animation
    const animateElements = document.querySelectorAll('.hero, .changelog, #latest-news, .feature-card, .footer-section, .stat-item');
    animateElements.forEach(el => observer.observe(el));
}

function initScrollEffects() {
    // Parallax effect for hero section
    window.addEventListener('scroll', function() {
        const scrolled = window.pageYOffset;
        const hero = document.querySelector('.hero');
        const codePreview = document.querySelector('.code-preview');

        if (hero) {
            hero.style.transform = `translateY(${scrolled * 0.3}px)`;
        }

        if (codePreview) {
            codePreview.style.transform = `translateY(${scrolled * -0.2}px) rotate(${scrolled * 0.01}deg)`;
        }
    });

    // Dynamic header background on scroll
    window.addEventListener('scroll', function() {
        const header = document.querySelector('header');
        const scrolled = window.pageYOffset;

        if (scrolled > 50) {
            header.style.background = 'rgba(26, 26, 46, 0.98)';
            header.style.boxShadow = '0 5px 30px rgba(0, 0, 0, 0.4)';
        } else {
            header.style.background = 'rgba(26, 26, 46, 0.95)';
            header.style.boxShadow = '0 2px 20px rgba(0, 0, 0, 0.3)';
        }
    });
}

function initHoverEffects() {
    // 3D tilt effect for cards (reduced intensity for better UX)
    const cards = document.querySelectorAll('.feature-card, .changelog-entry, .version-entry, .changelog-entry-modern');
    cards.forEach(card => {
        card.addEventListener('mousemove', function(e) {
            const rect = this.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;

            const centerX = rect.width / 2;
            const centerY = rect.height / 2;

            // Reduced tilt intensity for better user experience
            const rotateX = (y - centerY) / 20; // Changed from /10 to /20
            const rotateY = (centerX - x) / 20; // Changed from /10 to /20

            this.style.transform = `perspective(1000px) rotateX(${rotateX}deg) rotateY(${rotateY}deg) translateZ(5px)`;
        });

        card.addEventListener('mouseleave', function() {
            this.style.transform = 'perspective(1000px) rotateX(0deg) rotateY(0deg) translateZ(0px)';
        });
    });

    // Magnetic effect for buttons
    const buttons = document.querySelectorAll('.download-btn, .social-link, .view-more-btn');
    buttons.forEach(button => {
        button.addEventListener('mousemove', function(e) {
            const rect = this.getBoundingClientRect();
            const x = e.clientX - rect.left - rect.width / 2;
            const y = e.clientY - rect.top - rect.height / 2;

            this.style.transform = `translate(${x * 0.1}px, ${y * 0.1}px)`;
        });

        button.addEventListener('mouseleave', function() {
            this.style.transform = 'translate(0px, 0px)';
        });
    });
}

function initLoadingEffects() {
    // Typing effect for hero text
    const heroText = document.querySelector('.hero-description');
    if (heroText) {
        const text = heroText.textContent;
        heroText.textContent = '';
        let i = 0;
        const typeWriter = () => {
            if (i < text.length) {
                heroText.textContent += text.charAt(i);
                i++;
                setTimeout(typeWriter, 30); // Faster typing
            } else {
                // Add blinking cursor effect
                heroText.innerHTML += '<span class="cursor-blink">|</span>';
            }
        };
        setTimeout(typeWriter, 1500);
    }

    // Staggered feature highlights animation
    const highlights = document.querySelectorAll('.feature-highlight');
    highlights.forEach((highlight, index) => {
        setTimeout(() => {
            highlight.classList.add('highlight-active');
        }, 2000 + (index * 300));
    });

    // Add loading animation
    window.addEventListener('load', function() {
        document.body.classList.add('loaded');

        // Animate stats with counter effect
        const statNumbers = document.querySelectorAll('.stat-number');
        statNumbers.forEach(stat => {
            const targetText = stat.textContent.trim();
            if (!isNaN(targetText) && targetText !== '') {
                // Numeric counter
                const target = parseInt(targetText);
                animateCounter(stat, 0, target, 2000);
            } else {
                // Text animation - just fade in
                stat.style.opacity = '0';
                setTimeout(() => {
                    stat.style.transition = 'opacity 1s ease';
                    stat.style.opacity = '1';
                }, 1000);
            }
        });
    });
}

function animateCounter(element, start, end, duration) {
    const startTime = performance.now();

    function update(currentTime) {
        const elapsed = currentTime - startTime;
        const progress = Math.min(elapsed / duration, 1);

        const current = Math.floor(start + (end - start) * progress);
        element.textContent = current;

        if (progress < 1) {
            requestAnimationFrame(update);
        }
    }

    requestAnimationFrame(update);
}

// Enhanced particle effect background
function createParticles() {
    const particleContainer = document.createElement('div');
    particleContainer.className = 'particles';
    document.body.appendChild(particleContainer);

    for (let i = 0; i < 100; i++) {
        const particle = document.createElement('div');
        particle.className = 'particle';
        particle.style.left = Math.random() * 100 + '%';
        particle.style.animationDelay = Math.random() * 20 + 's';
        particle.style.animationDuration = (Math.random() * 10 + 10) + 's';
        particle.style.opacity = Math.random() * 0.5 + 0.1;
        particleContainer.appendChild(particle);
    }
}

// Enhanced mouse follow effect with trail
let mouseTrail = [];
function createMouseTrail() {
    for (let i = 0; i < 5; i++) {
        const trail = document.createElement('div');
        trail.className = 'mouse-trail';
        trail.style.opacity = (5 - i) * 0.1;
        document.body.appendChild(trail);
        mouseTrail.push(trail);
    }
}

document.addEventListener('mousemove', function(e) {
    const cursor = document.querySelector('.cursor');
    if (cursor) {
        cursor.style.left = e.clientX + 'px';
        cursor.style.top = e.clientY + 'px';
    }

    // Update mouse trail only if not in corner areas
    const cornerThreshold = 50; // pixels from edge
    const isInCorner = e.clientX < cornerThreshold || e.clientX > window.innerWidth - cornerThreshold ||
                      e.clientY < cornerThreshold || e.clientY > window.innerHeight - cornerThreshold;

    if (!isInCorner) {
        mouseTrail.forEach((trail, index) => {
            setTimeout(() => {
                trail.style.left = e.clientX + 'px';
                trail.style.top = e.clientY + 'px';
            }, index * 50);
        });
    }
});

// Roadmap functionality
function toggleMilestone(button) {
    let details = button.nextElementSibling;
    while (details && !details.classList.contains('milestone-details')) {
        details = details.nextElementSibling;
    }
    if (!details) return;

    const icon = button.querySelector('.expand-icon');

    if (details.style.display === 'none' || details.style.display === '') {
        details.style.display = 'block';
        icon.textContent = '▲';
        button.innerHTML = '<span class="expand-icon">▲</span> Hide Details';
    } else {
        details.style.display = 'none';
        icon.textContent = '▼';
        button.innerHTML = '<span class="expand-icon">▼</span> View Details';
    }
}

// Initialize particle effects
createParticles();
createMouseTrail();

// Add cursor element
const cursor = document.createElement('div');
cursor.className = 'cursor';
document.body.appendChild(cursor);

// CSS for additional animations (to be added to style.css)
const additionalCSS = `
.loaded {
    animation: fadeInBody 0.5s ease-out;
}

@keyframes fadeInBody {
    from { opacity: 0; }
    to { opacity: 1; }
}

.cursor {
    position: fixed;
    width: 20px;
    height: 20px;
    background: rgba(0, 170, 255, 0.5);
    border-radius: 50%;
    pointer-events: none;
    z-index: 9999;
    transition: transform 0.1s ease;
    mix-blend-mode: difference;
}

.particles {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    pointer-events: none;
    z-index: -1;
}

.particle {
    position: absolute;
    width: 2px;
    height: 2px;
    background: rgba(0, 170, 255, 0.3);
    border-radius: 50%;
    animation: float 20s linear infinite;
}

@keyframes float {
    0% { transform: translateY(100vh) rotate(0deg); opacity: 0; }
    10% { opacity: 1; }
    90% { opacity: 1; }
    100% { transform: translateY(-100vh) rotate(360deg); opacity: 0; }
}

.animate-in {
    animation: slideInUp 0.8s ease-out forwards;
}

@keyframes slideInUp {
    from {
        opacity: 0;
        transform: translateY(50px);
    }
    to {
        opacity: 1;
        transform: translateY(0);
    }
}
`;

// Inject additional CSS
const style = document.createElement('style');
style.textContent = additionalCSS;
document.head.appendChild(style);